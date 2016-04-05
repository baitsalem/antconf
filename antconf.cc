


#include <antconf/antconf.h>
#include <antconf/antconf_packet.h>
#include <random.h>
#include <cmu-trace.h>
#include <time.h>

#define max(a,b)        ( (a) > (b) ? (a) : (b) )
#define CURRENT_TIME     Scheduler::instance().clock()

//#define DEBUG
//#define ERROR
//#define SEND_ANTLIGHT
//#define VIEW_DATA
//#define RETURN_ANTLIGHT
//#define ROUTE_TABLE_OPERATION
//#define PHEROMONE_TABLE_OPERATION
//#define AGENT_FORWARD
//#define LOCAL_CONNECTIVITY
//#define ANTCONF_LOCAL_REPAIR

//pour l'accès a l'entète des agents de antconf
int hdr_antconf::offset_;


//classe pour la prise en charge de l'entète antconf dans le simulateur
static class ANTCONFHeaderClass : public PacketHeaderClass 
{
  public:
     ANTCONFHeaderClass() : PacketHeaderClass("PacketHeader/ANTCONF", sizeof(hdr_all_antconf)) 
	 {
	  bind_offset(&hdr_antconf::offset_);
	 } 
} class_rtProtoANTCONF_hdr;




//classe pour reproduire l'agent antconf dans l'interpreteur (tcl)
static class ANTCONFclass : public TclClass 
{
  public:
        ANTCONFclass() : TclClass("Agent/ANTCONF") {}
        TclObject* create(int argc, const char*const* argv) 
		{
           assert(argc == 5);
	       return (new ANTCONF((nsaddr_t) Address::instance().str2addr(argv[4])));
        }
} class_rtProtoANTCONF;


//la methode qui permet d'interagir avec le programme c++ en utilisant le langage tcl (pour le lançemant des commandes tcl)
int
ANTCONF::command(int argc, const char*const* argv) 
{
		if(argc == 2) 
		{
			Tcl& tcl = Tcl::instance();    
			if(strncasecmp(argv[1], "id", 2) == 0) 
			{
				tcl.resultf("%d", index);
				return TCL_OK;
			}    
			if(strncasecmp(argv[1], "start", 2) == 0) 
			{
                              
				//envoi periodique des agents 
			        lttimer.handle((Event*) 0);

				//timer pour la gestion des id des Agents qui passent par ce noeud
				btimer.handle((Event*) 0);

			//#ifndef ANTCONF_LINK_LAYER_DETECTION	
				//pour maintenir la connectivité locale en envoyant des messages hello		
				htimer.handle((Event*) 0);

				// purger la table des voisins
				ntimer.handle((Event*) 0);
			  //#endif // LINK LAYER DETECTION

			       //purger la table de routage
			       	rtimer.handle((Event*) 0);
			  	
				//purger la table de pheromone 
                                ptimer.handle((Event*) 0);
   			
				//incrementé les dst demandé (anciennté)
				dtimer.handle((Event*) 0);

				//purger les demandes des routes
				ratimer.handle((Event*) 0);
				
				return TCL_OK;
			}
		}
		else if(argc == 3)
		{
			if(strcmp(argv[1], "index") == 0) 
			{
				index = atoi(argv[2]);
				return TCL_OK;
			}
			else if(strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) 
			{
				logtarget = (Trace*) TclObject::lookup(argv[2]);
				if(logtarget == 0)
				return TCL_ERROR;
				return TCL_OK;
			}
			else if(strcmp(argv[1], "drop-target") == 0) 
			{
				int stat = rqueue.command(argc,argv);
				if (stat != TCL_OK) return stat;
				return Agent::command(argc, argv);
			}
			else if(strcmp(argv[1], "if-queue") == 0) 
			{
				ifqueue = (PriQueue*) TclObject::lookup(argv[2]);      
				if(ifqueue == 0)
				return TCL_ERROR;
				return TCL_OK;
			}
			else if (strcmp(argv[1], "port-dmux") == 0)
			{
    			dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
				if (dmux_ == 0) 
				{
					fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__,
					argv[1], argv[2]);
					return TCL_ERROR;
				}
				return TCL_OK;
			}/**/
	}/**/

	return Agent::command(argc, argv);

}
//********************************************Constructeur***********************//
ANTCONF::ANTCONF(nsaddr_t id) : Agent(PT_ANTCONF), btimer(this), htimer(this), ntimer(this),  rtimer(this), lttimer(this), dtimer(this), ptimer(this), lrtimer(this), ratimer(this), rqueue() 
{
  index = id;
  seqno = 2;
  idAgent = 1;
  askRt= false;
  nb_num=0;
  dem_num=0;

  //liste des voisins
  LIST_INIT(&nbhead);
  
  //liste des agents passés par ce noeud
  LIST_INIT(&bihead);
  
  //liste des demandes des routes
  LIST_INIT(&ARhead);
  logtarget = 0;
  ifqueue = 0;
//  srand(time(NULL));
}


/*
  Timers
*/
/* timer pour Purger la table des Agents ID*/
void
AgentIdTimer::handle(Event*) 
{
  agent->id_purge(); 
  Scheduler::instance().schedule(this, &intr, AGENT_ID_SAVE);
}

/* Envoi periodique des messages hello pour maintenir la connectivitée locale*/
void
HelloTimerAnt::handle(Event*) 
{
   agent->sendHello(); 
   double interval = MinHelloInterval + ((MaxHelloInterval - MinHelloInterval) * Random::uniform());
   assert(interval >= 0);
   Scheduler::instance().schedule(this, &intr, interval);
}

/*-------------------------------Purger la table des voisins--------------*/
void
NeighborTimerAnt::handle(Event*) 
{
  agent->nb_purge();
  Scheduler::instance().schedule(this, &intr, HELLO_INTERVAL);
}

/*-------------------envoie periodique des agents -------------------*/ 
void
LightTimerAnt::handle(Event*) 
{
  agent->sendAntLight(); 
  //double interval1 = MinAntSendInterval + ((MaxAntSendInterval - MinAntSendInterval) * Random::uniform()); 
  Scheduler::instance().schedule(this, &intr, 3.0);
}
 
/* Purger la table de routage */
void
RouteCacheTimerAnt::handle(Event*) 
{
  agent->rt_purge();
  #define FREQUENCY 0.1 // sec
  Scheduler::instance().schedule(this, &intr, FREQUENCY);
}

/*Purger la table de pheromone c a d diminuer la qte du pheromne, si qte=0 alors supprimé l'entrée de la table*/
void
PheromoneTableTimer::handle(Event*) 
{
  agent->pht_purge();
  #define INTERV  1// sec
  Scheduler::instance().schedule(this, &intr, INTERV);
}


/*Purger la table de demandes c a d augmenté la qte du pheromne pour mesurer l'ancienneté de la dst demandée*/
void
PheromoneDemandeTableTimer::handle(Event*) 
{
  agent->phd_purge();
  #define INTERVD  0.2// sec
  Scheduler::instance().schedule(this, &intr, INTERVD);
}


/*-------------desactiver l'entrée si elle n'est pas valide en attendant sa reparation------------*/
void
LocalRepairTimerAnt::handle(Event* p)  
{ 
   antconf_rt_entry *rt;
   struct hdr_ip *ih = HDR_IP( (Packet *)p);
   rt = agent->rtable.rt_lookup(ih->daddr());	
   if (rt && rt->rt_flags != RTF_UP) 
   {
      agent->rt_down(rt);      
   }
    Packet::free((Packet *)p);
}

/***************************purger la liste des agents passsés par ce noeud***************/
void
RouteAskTimer::handle(Event*) 
{
  //agent->ar_purge();
  #define INTERVAL 5 // sec
  Scheduler::instance().schedule(this, &intr, INTERVAL);
}


//inserer une entrer  ip:idagent dans la listes des agents 
void
ANTCONF::id_insert(nsaddr_t id, u_int32_t idAgent) 
{
  AgentID *b = new AgentID(id, idAgent);
  assert(b);
  b->expire = CURRENT_TIME + AGENT_ID_SAVE;
  LIST_INSERT_HEAD(&bihead, b, link);
}


//verifier si le couple ip:idagent existe dans la cache des AgentID
bool
ANTCONF::id_lookup(nsaddr_t id, u_int32_t idAgent) 
{
  AgentID *b = bihead.lh_first;   
  for( ; b; b = b->link.le_next) 
  {
   if ((b->src == id) && (b->id == idAgent))   return true;     
  }
 return false;
}

//purger la table de des agents en éliminant les entrées expirées
void
ANTCONF::id_purge() 
{	
        AgentID *b = bihead.lh_first;
	AgentID *bn;
	double now = CURRENT_TIME;	
	for(; b; b = bn) 
	{
		bn = b->link.le_next;
		if(b->expire <= now) 
		{
			
			LIST_REMOVE(b,link);
			delete b;
		}
	}
}

//purger la table des pheromones
void
ANTCONF::pht_purge()
{
   antconf_phtable_entry *rt, *rtn;
   for(rt = phtable.head(); rt; rt = rtn) 
   { 
     rtn = rt->pht_link.le_next;
     rt->rph_qte-=DEC_PHEROMONE;
     if(rt->rph_qte<=0) 
     {
	#ifdef PHEROMONE_TABLE_OPERATION
		fprintf(stderr, "- %s: i am %d   route pheromone delete: demandeur: %d destination demandée: %d at %2f\n", __FUNCTION__, index,  rt->rph_claimant, rt->rph_dsttoask, Scheduler::instance().clock());
	#endif
	phtable.rtph_delete(rt->rph_claimant, rt->rph_dsttoask); 
     }
   }

}


/*
  gestion des reptures des liens
*/
static void
antconf_rt_failed_callback(Packet *p, void *arg) 
{
  ((ANTCONF*) arg)->linklayer_failed(p);
}

/*methode appelée quand une repture d'un lien au moment de l'envoie des données est detectée */
void
ANTCONF::linklayer_failed(Packet *p) 
{	
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	antconf_rt_entry *rt;
	//fprintf(stderr, "%s: calling drop() linklayer_failed\n", __FUNCTION__);	 
	nsaddr_t broken_nbr = ch->next_hop_;
	#ifndef ANTCONF_LINK_LAYER_DETECTION
	drop(p, DROP_RTR_MAC_CALLBACK);
	//fprintf(stderr, "%s: calling drop() linklayer_failed\n", __FUNCTION__);
	#else
	if(! DATA_PACKET(ch->ptype()) || (u_int32_t) ih->daddr() == IP_BROADCAST) 
	{
		drop(p, DROP_RTR_MAC_CALLBACK);
		return;
	}
	log_link_broke(p);
	if((rt = rtable.rt_lookup(ih->daddr())) == 0) 
	{
		drop(p, DROP_RTR_MAC_CALLBACK);
		return;
	}
	log_link_del(ch->next_hop_);
	#ifdef ANTCONF_LOCAL_REPAIR
	
		rt_repair(rt, p); // local repair		
		return;
		
	#endif // LOCAL REPAIR	
	{
		drop(p, DROP_RTR_MAC_CALLBACK);		
		while((p = ifqueue->filter(broken_nbr))) 
		{
			drop(p, DROP_RTR_MAC_CALLBACK);
		}	
		nb_delete(broken_nbr);
	}
	#endif // LINK LAYER DETECTION
}

/***************** methode appelée quand le lien avec un voisin est cassé********************/
void
ANTCONF::link_failure_management(nsaddr_t id) 
{
  antconf_rt_entry *rt, *rtn;
  Packet *rerr = Packet::alloc();
  struct hdr_antconf_rectificator *re = HDR_ANTCONF_RECTIFICATOR(rerr);
  re->DestCount = 0;
  for(rt = rtable.head(); rt; rt = rtn) 
  {  
     rtn = rt->rt_link.le_next; 
     if ((rt->rt_hops != INFINITY2) && (rt->rt_nexthop == id) ) 
     {
        assert (rt->rt_flags == RTF_UP);
        assert((rt->rt_seqno%2) == 0);
        rt->rt_seqno++;
        re->unreachable_dst[re->DestCount] = rt->rt_dst;
        re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
        #ifdef ERROR
          fprintf(stderr, "%s(%f): %d\t(%d\t%u\t%d)\n", __FUNCTION__, CURRENT_TIME,
		     index, re->unreachable_dst[re->DestCount],
		     re->unreachable_dst_seqno[re->DestCount], rt->rt_nexthop);
        #endif 
        re->DestCount += 1;
        rt_down(rt);
     }
     // supprimer le voisin dans listv de l'entrée (de la table de routage)
     rt->pc_delete(id);
  }   
  if (re->DestCount > 0) 
  {
    #ifdef ERROR
       fprintf(stderr, "%s(%f): %d\t sending Rectificator Agent From link failer management at %2f...\n", __FUNCTION__, CURRENT_TIME, index,Scheduler::instance().clock());
    #endif 
    sendRectificatorAgent(rerr, false);
  }
  else 
  {
    Packet::free(rerr);
  }
}


//marquer une route en reparation
void
ANTCONF::rt_repair(antconf_rt_entry *rt, Packet *p) 
{
	#ifdef ANTCONF_LOCAL_REPAIR
		fprintf(stderr, " %s: i am %d put route in repair: nexthop is  %d    destination: %d hopcount: %d  at % 2f\n",  __FUNCTION__, index, rt->rt_nexthop, rt->rt_dst, rt->rt_hops, Scheduler::instance().clock());
        #endif
	//Bufferiser le paquet 
	rqueue.enque(p);

	//marquer la route comme en reparation
	rt->rt_flags = RTF_IN_REPAIR;
	if (!ar_lookup(rt->rt_dst))
	{
		askRt=true;
		ar_insert(rt->rt_dst,0);
	}
	int rt_req_timeout = 1;

	//set up a timer interrupt
	Scheduler::instance().schedule(&lrtimer, p->copy(), rt_req_timeout);
}


/* *****************************MAJ table de routage*****************************************/
void
ANTCONF::rt_update(antconf_rt_entry *rt, u_int32_t seqnum, u_int16_t metric, double expire_time) 
{
     rt->rt_seqno = seqnum;
     rt->rt_hops = metric;
     rt->rt_flags = RTF_UP;     
     rt->rt_expire = expire_time;
}


//marquer une entrée de la table de routage comme expirée
void
ANTCONF::rt_down(antconf_rt_entry *rt) 
{
  if(rt->rt_flags == RTF_DOWN) 
  {
    return;
  }

  #ifdef ROUTE_TABLE_OPERATION
	fprintf(stderr, " %s: i am %d put route down nexthop is  %d    destination: %d hopcount: %d  at % 2f\n",  __FUNCTION__, index, rt->rt_nexthop, rt->rt_dst, rt->rt_hops, Scheduler::instance().clock());
  #endif

  //rt->rt_last_hop_count = rt->rt_hops;
  rt->rt_hops = INFINITY2;
  rt->rt_flags = RTF_DOWN;
  rt->rt_nexthop = -1;
  rt->rt_expire = 0;

} 


//purger la table de routage (desactiver les entrées expirées)
void
ANTCONF::rt_purge() 
{
	antconf_rt_entry *rt, *rtn;
	double now = CURRENT_TIME;
	double delay = 0.0;
	Packet *p;
	for(rt = rtable.head(); rt; rt = rtn) 
	{ 
		rtn = rt->rt_link.le_next;
		if ((rt->rt_flags == RTF_UP) && (rt->rt_expire < now)) 
		{			
			assert(rt->rt_hops != INFINITY2);
			while((p = rqueue.deque(rt->rt_dst))) 
			{
				//#ifdef DEBUG
				//fprintf(stderr, "%s: calling drop purge()\n", __FUNCTION__);
				//#endif 
				drop(p, DROP_RTR_NO_ROUTE);
			}
			rt->rt_seqno++;
			assert (rt->rt_seqno%2);
			rt_down(rt);
		}
		else if (rt->rt_flags == RTF_UP) 
		{			
			assert(rt->rt_hops != INFINITY2);
			while((p = rqueue.deque(rt->rt_dst))) 
			{
			//fprintf(stderr, "%s: je depile pour dest  ()\n", __FUNCTION__);
				forward (rt, p, delay);
				delay += ARP_DELAY;												
			}
		} 
		else if (rqueue.find(rt->rt_dst))		
		if (!ar_lookup(rt->rt_dst))
	        {
                  askRt=true;
		  ar_insert(rt->rt_dst,0);
	        } 
   	}
}

/******************************routine de Reception d'un paquet ****************************/
void
ANTCONF::recv(Packet *p, Handler*) 
{
   struct hdr_cmn *ch = HDR_CMN(p);
   struct hdr_ip *ih = HDR_IP(p);
   
   //si c'est un agent
  if(ch->ptype() == PT_ANTCONF) 
  {
     //Tcl::instance().evalf("forward_request %d %d ",ih->saddr(),ih->daddr());
     ih->ttl_ -= 1;
     recvANTCONF(p);
     return;
  }
  //c'est mon paquet
  if((ih->saddr() == index) && (ch->num_forwards() == 0)) 
  {  
     /* ajout d'entete*/     
     ch->size() += IP_HDR_LEN;
     if ( (u_int32_t)ih->daddr() != IP_BROADCAST) ih->ttl_ = NETWORK_DIAMETER;
  }
  else if(ih->saddr() == index) 
  {                                            
    drop(p, DROP_RTR_ROUTE_LOOP);
    //fprintf(stderr, "%s: calling drop() recvvvvvvvv\n", __FUNCTION__);
    return;
  }
  else 
  {    
    if(--ih->ttl_ == 0) 
    {
       //fprintf(stderr, "%s: calling drop() recv ttl\n", __FUNCTION__);
	drop(p, DROP_RTR_TTL);
       return;
    }
  }
   if ( (u_int32_t)ih->daddr() != IP_BROADCAST)
  {
	#ifdef VIEW_DATA
	fprintf(stderr, "i am %d receving data packet from %d to %d  at %.2f \n",index, ih->saddr(),ih->daddr(), Scheduler::instance().clock());
	#endif
	rt_resolution(p);
  } 
   else
   forward((antconf_rt_entry*) 0, p, NO_DELAY);
}

/*gestion des routes*/
void
ANTCONF::rt_resolution(Packet *p) 
{	
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	antconf_rt_entry *rt;
	
	ch->xmit_failure_ = antconf_rt_failed_callback;
	ch->xmit_failure_data_ = (void*) this;
	if(nb_lookup(ih->daddr()))
	{
		
		ch->next_hop_ = ih->daddr();
		ch->addr_type() = NS_AF_INET;
		ch->direction() = hdr_cmn::DOWN;
		ih->daddr()=ih->daddr();
		//fprintf(stderr, "%s: ia am %d sending paquet data source %d to dest %d neighobrd at %2f. ..\n", __FUNCTION__,index,ih->saddr(),ih->daddr(),Scheduler::instance().clock());
		Scheduler::instance().schedule(target_, p, 0.);
		
	}else
	{
	rt = rtable.rt_lookupoptimal(ih->daddr());
	if(rt == 0) 
	{	
		rt = rtable.rt_add(ih->daddr());
	}		
	if(rt->rt_flags == RTF_UP) 
	{
		//fprintf(stderr, "%s: je suis %d envoi directt a %d  de %d at %2f...\n", __FUNCTION__,index,ih->daddr(),ih->saddr(),Scheduler::instance().clock());
		forward(rt, p, NO_DELAY);
	}
	else if(ih->saddr() == index) 
	{
		//fprintf(stderr, "%s: ia am %d empiling for resolution at %2f...\n", __FUNCTION__,index,Scheduler::instance().clock());
		rqueue.enque(p);
		if (!ar_lookup(ih->daddr()))
		{
                       	//fprintf(stderr, "%s: ia am %d je demande une route vers %d  at %2f...\n", __FUNCTION__,index,ih->daddr(),Scheduler::instance().clock());
			askRt=true;			
			ar_insert(ih->daddr(),0);
		}
	}
	else if (rt->rt_flags == RTF_IN_REPAIR) 
	{
		rqueue.enque(p);
	}
	 else 
	{
		Packet *rerr = Packet::alloc();
		struct hdr_antconf_rectificator *re = HDR_ANTCONF_RECTIFICATOR(rerr);
		assert (rt->rt_flags == RTF_DOWN);
		re->DestCount = 0;
		re->unreachable_dst[re->DestCount] = rt->rt_dst;
		re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
		re->DestCount += 1;
		#ifdef ERROR
			fprintf(stderr, "%s: ia am %d sending rectificator agent for resolution at %2f...\n", __FUNCTION__,index,Scheduler::instance().clock());
		#endif
		sendRectificatorAgent(rerr, false);
		fprintf(stderr, "%s: calling drop() resolution relaisss\n", __FUNCTION__);
		//rqueue.enque(p);
		drop(p, DROP_RTR_NO_ROUTE);
	}}
}

//forwarder le paquet
void
ANTCONF::forward(antconf_rt_entry *rt, Packet *p, double delay) 
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);

	if(ih->ttl_ == 0) 
	{		
		fprintf(stderr, "%s: calling drop() forward\n", __FUNCTION__);
		drop(p, DROP_RTR_TTL);
		return;
	}
	if (ch->ptype() != PT_ANTCONF && ch->direction() == hdr_cmn::UP &&	((u_int32_t)ih->daddr() == IP_BROADCAST)|| (ih->daddr() == here_.addr_)) 
	{
		dmux_->recv(p,0);
		return;
	}
	if (rt) 
	{
		assert(rt->rt_flags == RTF_UP);
		rt->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
		ch->next_hop_ = rt->rt_nexthop;
		ch->addr_type() = NS_AF_INET;
		ch->direction() = hdr_cmn::DOWN;       
	}
	else 
	{ 
		assert(ih->daddr() == (nsaddr_t) IP_BROADCAST);
		ch->addr_type() = NS_AF_NONE;
		ch->direction() = hdr_cmn::DOWN;
	}
	if (ih->daddr() == (nsaddr_t) IP_BROADCAST) 
	{
		// si c'est une diffusion
		assert(rt == 0);
		Scheduler::instance().schedule(target_, p, 0.01 * Random::uniform());
	}
	else 
	{ 
		if(delay > 0.0) 
		{
			Scheduler::instance().schedule(target_, p, delay);
		}
		else 
		{
			Scheduler::instance().schedule(target_, p, 0.);
		}
	}
}


/* ********************************Reception d'un paquet  ANTCONF*******************************/
void
ANTCONF::recvANTCONF(Packet *p) 
{
  
  struct hdr_antconf *ah = HDR_ANTCONF(p);
  assert(HDR_IP (p)->sport() == RT_PORT);
  assert(HDR_IP (p)->dport() == RT_PORT);
  /*
  * arrivé d'un paquet antconf
  */
  switch(ah->ah_type) 
  {
     case ANTCONFTYPE_ANTLIGHTGO:

	recvAntLightGo(p);
	
     break;

     case ANTCONFTYPE_ANTLIGHTRETURN:		
     	recvAntLightReturn(p);
     break;

    case ANTCONFTYPE_RECTIFICATOR:
     	recvRectificatorAgent(p);
     break; 

     case ANTCONFTYPE_HELLO:

     	recvHello(p);
     break;
        
     default:
     fprintf(stderr, "Invalid ANTCONF type (%x)\n", ah->ah_type);
     exit(1);
  }

}

void
ANTCONF::sendAntLight() 
{
	 Packet *p = Packet::alloc();
         struct hdr_cmn *ch = HDR_CMN(p);
	 struct hdr_ip *ih = HDR_IP(p);
	 struct hdr_antconf_antlight *antL = HDR_ANTCONF_ANTLIGHT(p);
	 
         ih->ttl_ =10;
	 nsaddr_t nexthp=-1;
	 int nb_v=0;
	 int i=0;
	 	
         //les champs de l'agent leger
	 antL->al_type = ANTCONFTYPE_ANTLIGHTGO;
         antL->antL_hop_count = 1;
	 antL->scr = index;	
	 antL->antL_id = idAgent++;	
 	 seqno += 2;
	 assert ((seqno%2) == 0);
       	 antL->antL_src_seqno=seqno;
         antL->antL_prevnode=index;
	 antL->antL_nodetarget=-1;
         antL->antL_detask=-1;
         antL->antL_NoeudEtabRout=index;
         antL->antL_validPrevNER=true;
	 antL->NodeVisitedCount = 0;
	 antL->antL_rt=0;
	 antL->ph_qte=0;
         /*ANTCONF_Neighbor *nb1 = nbhead.lh_first;
	 for(; nb1; nb1 = nb1->nb_link.le_next) 
         {
		nexthp=nb1->nb_addr;
		break;
	 }*/
         if(nb_num>0)
	 {
                nb_v=rand()%nb_num;
                if(nb_v==0) nb_v=1;
                ANTCONF_Neighbor *nb1 = nbhead.lh_first;
		for(; nb1; nb1 = nb1->nb_link.le_next) 
         	{
			i++;
			if(i==nb_v)
			{
				nexthp=nb1->nb_addr;
				break;
			}
		}
		//if(index==0)fprintf(stderr, "i am %d(%d) my next hopt is %d   at %.2f port %d\n", index,idAgent,nexthp, Scheduler::instance().clock(),RT_PORT);
	 }
	 if(nexthp==-1) 
	{
	    	Packet::free(p);
	    	return;	
	}	
	antL->NodeVisited[antL->NodeVisitedCount] = index;
	antL->NodeVisitedCount += 1;
	antL->antL_nexthop = nexthp;
	ch->ptype() = PT_ANTCONF;
	ch->size() = IP_HDR_LEN + antL->size();
	ch->iface() = -2;
	ch->error() = 0;
	ch->addr_type() = NS_AF_NONE;
	ch->prev_hop_ = index;
    	ch->next_hop_ = nexthp;
	ch->direction() = hdr_cmn::DOWN; 
	//#ifdef SEND_ANTLIGHT
	/*if(index==0)
	   	fprintf(stderr, "i am %d(%d) my next hopt is %d   at %.2f port %d\n", index,idAgent,nexthp, Scheduler::instance().clock(),RT_PORT);*/
        //#endif
	
	ih->saddr() = index;
	ih->daddr() = nexthp;	
	 
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;	
        Scheduler::instance().schedule(target_, p, 0.);
}


/* ********************************Reception d'un agent dans sa phase aller*********************************************/
void
ANTCONF::recvAntLightGo(Packet *p) 
{
    struct hdr_antconf_antlight *antL = HDR_ANTCONF_ANTLIGHT(p);
    struct hdr_cmn *ch = HDR_CMN(p);
    struct hdr_ip *ih = HDR_IP(p);
    //if (antL->antL_nodetarget !=-1) 
//	Tcl::instance().evalf("forward_demande %d  ",index);
   /*supprimmer si je suis l'envoyeur de l'agent*/
   if((index)==(antL->scr)) 
   {
      #ifdef DEBUG
        fprintf(stderr, "%s: i am %d it is my ANTLIGHT agent I come back from %d, hop count is %d\n", __FUNCTION__,index,antL->antL_prevnode,antL->antL_hop_count);
      #endif 
      Packet::free(p);
      return;
   }
   /*supprimmer si j'ai deja recu cet agent*/
   if (id_lookup(antL->scr, antL->antL_id)) 
   {
     #ifdef DEBUG
        fprintf(stderr, "%s:  je suis %d agent antlight deja recu je viens de %d, etape %d\n", __FUNCTION__,index, antL->antL_prevnode,antL->antL_hop_count);
     #endif 
     Packet::free(p);
     return;
   }
   /* cacher l'identifiant de l'agent avec l'adresse source */
   id_insert(antL->scr, antL->antL_id);
  /* on va soit relier l'agent ou le retourné. mais avant il faut etablir une route vers NER  si les conditions sont satisfaite  */ 
  if((antL->antL_validPrevNER==true)&&(nb_lookup(antL->antL_prevnode)))
	//if((antL->antL_validPrevNER==true))
	//for(int k=0;k<antL->NodeVisitedCount;k++)
        
	//if ((antL->antL_nodetarget ==index)&& (antL->antL_rt == 0)) fprintf(stderr, "%s:  je suis %d je trouve des route pour la destination %d intermidiaireeeeeeeeeeeeeeeeeeeeeeeee \n", __FUNCTION__,index, antL->antL_validPrevNER);
	{	
		if(ar_lookup(antL->antL_NoeudEtabRout)) 
		{
			ar_delete(antL->antL_NoeudEtabRout);
			if(ar_empty()) askRt=false;
		}
		antconf_rt_entry *rt0;	
		// antL0 est la route vers NER
		rt0 = rtable.rt_lookup(antL->antL_prevnode, antL->antL_NoeudEtabRout);
		/* si la route n'existe pas créer une*/
		if(rt0 == 0) 
		{
			rt0 = rtable.rt_add(antL->antL_prevnode, antL->antL_NoeudEtabRout);
		}else 
		//if ((antL->antL_nodetarget ==index)&& (antL->antL_rt == 1)) fprintf(stderr, "%s:  je suis %d je trouve des route pour la destination %d intermidiaireeeeeeeeeeeeeeeeeeeeeeeee %d\n", __FUNCTION__,index, rt0->rt_dst,rt0->rt_hops);
                rt0->rt_expire = max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE));
		if ( (antL->antL_src_seqno > rt0->rt_seqno ) ||((antL->antL_src_seqno == rt0->rt_seqno) &&  (antL->antL_hop_count<rt0->rt_hops)) ) 
		{
			//if ((antL->antL_nodetarget ==index)&& (antL->antL_rt == 0)) fprintf(stderr, "%s:  je suis %d je trouve des route pour la destination %d intermidiaireeeeeeeeeeeeeeeeeeeeeeeee %d\n", __FUNCTION__,index, rt0->rt_dst,rt0->rt_hops);
			rt_update(rt0, antL->antL_src_seqno, antL->antL_hop_count, max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE)) );//apres ajouter la confiance nc
			assert (rt0->rt_flags == RTF_UP);
			Packet *buffered_pkt;
			while ((buffered_pkt = rqueue.deque(rt0->rt_dst))) 
			{
				if (rt0 && (rt0->rt_flags == RTF_UP)) 
				{
				//if ((antL->antL_nodetarget ==index)&& (antL->antL_rt == 1)) fprintf(stderr, "%s:  je suis %d je trouve des route pour la destination %d intermidiaireeeeeeeeeeeeeeeeeeeeeeeee %d\n", __FUNCTION__,index, rt0->rt_dst,rt0->rt_hops);
					assert(rt0->rt_hops != INFINITY2);
				forward(rt0, buffered_pkt, NO_DELAY);
				}
			}
		 }
        }
  if(antL->antL_rt)
  {
	if(antL->antL_nodetarget==index)
	{
	 Packet::free(p);
     	 return;
	}
	antL->antL_prevnode=index; 
	antL->antL_hop_count += 1;;
	antconf_rt_entry *rt;
	rt = rtable.rt_lookupoptimal(antL->antL_nodetarget);
	if (rt!=0)
	{
        	antL->antL_nexthop=rt->rt_nexthop;
		ch->next_hop_ =rt->rt_nexthop;
        	ch->addr_type() = NS_AF_INET;
        	ch->direction() = hdr_cmn::DOWN;
        	ih->daddr() = antL->antL_nodetarget;
		Scheduler::instance().schedule(target_, p, DELAY);
	}	
  }else 	
  chooseNextHop(p) ;
}


/* ********************************choix du prochain neoud*********************************************/
void
ANTCONF::chooseNextHop(Packet *p) 
{
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_antconf_antlight *antL = HDR_ANTCONF_ANTLIGHT(p);

	nsaddr_t nexthoop=-12;
	nsaddr_t nexthop=-12;
	u_int8_t nc;
        nc=antL->NodeVisitedCount-1;	
	if (antL->antL_nodetarget !=-1)
	{
                
		//if (antL->antL_rt==0) fprintf(stderr, " %s: je viens de %d je suis a %d demandeurrrrrrrrrrrrrrrrrrrr %d qui demande %d\n",  __FUNCTION__, antL->scr, index,antL->antL_nodetarget,antL->antL_NoeudEtabRout);
		//fprintf(stderr, " index %d %s: sssssssssssssssssssssssssssssssssssssssssssssssssss\n",  index,__FUNCTION__);
		if (antL->antL_nodetarget ==index)
		{
		     //if (antL->antL_rt==0) fprintf(stderr, " %s: je viens de %d je suis a %d demandeurrrrrrrrrrrrrrrrrrrr qui demande %d\n",  __FUNCTION__, antL->scr, index,antL->antL_NoeudEtabRout);
					
		     sendAntLightReturn(antL->NodeVisitedCount,antL->NodeVisited,antL->scr, antL->antL_NoeudEtabRout);
		     //sendAntLightReturn(antL->NodeVisitedCount,antL->NodeVisited,antL->scr);
		     //if  (ih->ttl_!=0) sendAntLightClone(p);
                     Packet::free(p);
		     return;
		}
		else
		{
			antconf_phtable_entry *phrt;
			ANTCONF_NeighborPh  *nbph;
			
                        phrt=phtable.rtph_lookup(antL->antL_nodetarget, antL->antL_NoeudEtabRout);

			if(phrt)
			{			
                             nbph=phrt->rtph_nblist.lh_first;			
			     nexthoop=nbph->nbph_addr;
			}			
		
			if(antL->antL_NoeudEtabRout==index)
			{
				antL->antL_hop_count=0 ;
				antL->antL_src_seqno=seqno;
				antL->antL_validPrevNER=true ; 
			}
			else
			{
					u_int32_t  seq=0;
					bool Nodevisited=false;
					bool valider=false;
					u_int16_t d=10000;
					antconf_rt_entry *rt;
					nsaddr_t dst;
										
					ANTCONF_Neighbor *nb = nbhead.lh_first;

					for(; nb; nb = nb->nb_link.le_next) 
					{         rt = rtable.rthead.lh_first;
						for(; rt; rt = rt->rt_link.le_next) 
						{
							if ((rt->rt_dst == antL->antL_NoeudEtabRout)&&(rt->rt_nexthop==nb->nb_addr)&&(nb->nb_addr==nexthoop))
							{
								Nodevisited=true;
								break;
							}
							if(((rt->rt_dst == antL->antL_NoeudEtabRout)&&(rt->rt_nexthop==nb->nb_addr))&&((rt->rt_seqno>seq)||((rt->rt_seqno==seq)&&(rt->rt_hops<d))))
							{			   			  
								antL->antL_hop_count=rt->rt_hops ;
								antL->antL_src_seqno = rt->rt_seqno ;
								antL->antL_validPrevNER =true ;	
								nexthop=rt->rt_nexthop;
								dst=rt->rt_dst;
								valider=true;
							}
						}
		  				if (Nodevisited) break;		  	    
					}
					if((!valider)||(Nodevisited)) antL->antL_validPrevNER =false ;	
					if(antL->antL_validPrevNER==true)
					{
					    antconf_rt_entry *ln= rtable.rt_lookup(nexthop,dst);
					    if(ln)	
					        ln->pc_insert(nexthoop);
					}
		        }
               }
        }
       else if ((askRt)&&(index !=antL->scr))
       { 
	  	//if (index==12) 
		//fprintf(stderr, " %s: i am %d(%d) j'ai trouvé un demandeur  source %d at % 2f  target %d visited %d ttl %d nbbbbbb %d\n",  __FUNCTION__, index,idAgent,antL->scr,Scheduler::instance().clock(),antL->antL_nodetarget,antL->NodeVisitedCount, ih->ttl_, antL->NodeVisited[1]);
	 
	 	sendAntLightReturn(antL->NodeVisitedCount,antL->NodeVisited,antL->scr,-1);
         	if  (ih->ttl_!=0) sendAntLightClone(p);
	 	Packet::free(p);
	 	return;
       }
       else if  (ih->ttl_==0)
       {
		//fprintf(stderr, " %s: ttlllllllllllllllllllllll=0\n",  __FUNCTION__);
		sendAntLightReturn(antL->NodeVisitedCount,antL->NodeVisited,antL->scr, -1);
                Packet::free(p);
		return;
       }
       else // L'agent est toujours dans sa phase aller
       {	
       int QtePh=0;
       nsaddr_t claimant;
       nsaddr_t dstask;
       nsaddr_t claimanttab[1000];
       nsaddr_t dstasktab[1000];
       nsaddr_t nexthooptab[1000];
       bool IntablePh=false;
       bool NotIntablePh=false;
       bool nb_inph=false;
       bool nosearch=false;
       nsaddr_t nbnotinphtab[1000];
       
       bool premier=true;
       int x=0;
       int j=0; 
       int i=0;	
       int g=0;
       int taille=0;
       int randam=0;	
       antconf_phtable_entry *rt;
       ANTCONF_Neighbor *nb = nbhead.lh_first;
       for(; nb; nb = nb->nb_link.le_next)
       {
	     nb_inph=false;
	     rt=phtable.phthead.lh_first;
             for(; rt; rt = rt->pht_link.le_next)
	     {
		if(rt->nbph_lookup(nb->nb_addr)) 
		{			
			nb_inph=true;
			QtePh=int(rt->rph_qte*10);
			IntablePh=true;
			if(QtePh>0)
			{
				if(premier)
				{
					for(i=0; i<=QtePh; i++)
	                		{
						claimanttab[i]=rt->rph_claimant;
						dstasktab[i]=rt->rph_dsttoask;
						nexthooptab[i]=nb->nb_addr;
						j=i;
						taille=i;
					}
					premier=false;
		        	}else
				{
                        		for(g=j+1; g<=(QtePh+j+1); g++)
	                		{
						claimanttab[g]=rt->rph_claimant;
						dstasktab[g]=rt->rph_dsttoask;
						nexthooptab[g]=nb->nb_addr;
						taille=g;
						if (g==QtePh+j+1) 
						{ j=g;break;}
					}
				}
			}else nb_inph=false;
		}
	     }
	     if(!nb_inph) 
	     {
		nbnotinphtab[x]=nb->nb_addr;
		x++;
	     }
        }
	
	if (x>0)
	{
	     for(i=1;i<x;i++)
	     {
		nexthooptab[taille+i]=nbnotinphtab[i];
		dstasktab[taille+i]=-1;
		claimanttab[taille+i]=-1;
	     }	
	     taille=taille+x;
	}
	if(taille==0) IntablePh=false;
	if (IntablePh)
        {
                randam=rand()%taille;
		if(claimanttab[randam]==-1)
		{
			nosearch=true;
			IntablePh=false;
                	nexthoop=nexthooptab[randam];
		}else
		{
                	nexthoop=nexthooptab[randam];
			claimant=claimanttab[randam];
			dstask=dstasktab[randam];
			fprintf(stderr, " %s: je suis %d vers %d pour satisfaire %d qui demande %d taille %d j %d clone\n",  __FUNCTION__,index,nexthoop,claimant,dstask,taille,j);		
		}
		//Tcl::instance().evalf("forward_demande %d  ",index);
	}
        if (!IntablePh)
        {
        	NotIntablePh=true;
		if(nosearch)
		{
			nexthoop=nexthooptab[randam];
			
		}else
		{
			ANTCONF_Neighbor *nb0 = nbhead.lh_first;
			for(; nb0; nb0 = nb0->nb_link.le_next) 
        		{
         			if((nb0->nb_addr!=antL->antL_prevnode)&&(nb0->nb_addr!=antL->scr)&&(!(visitedNode(nb0->nb_addr,antL->NodeVisited,antL->NodeVisitedCount)))) 
         			{
                			nexthoop=nb0->nb_addr;
                			break;
         			}
      			}
		}
 	}
        //if((!IntablePh)&&(!NotIntablePh)) nexthoop=-12;
	  	if(IntablePh)
		{
			sendAntLightClone(p);
			antL->antL_nodetarget= claimant ;
			antL->antL_NoeudEtabRout= dstask;
			ih->ttl_=10; 
			if(dstask==index)
			{
				antL->antL_hop_count=0 ;
				antL->antL_src_seqno=seqno;
				antL->antL_validPrevNER=true ; 
			}
			else
		        {
				u_int32_t seq=0;
				bool Nodevisited=false;
				bool valider=false;
				u_int16_t d=10000;
				nsaddr_t dst;					
				antconf_rt_entry *rt;
				ANTCONF_Neighbor *nb = nbhead.lh_first;
				for(; nb; nb = nb->nb_link.le_next) 
				{        rt = rtable.rthead.lh_first;
					for(; rt; rt = rt->rt_link.le_next) 
					{
						if ((rt->rt_dst == antL->antL_NoeudEtabRout)&&(rt->rt_nexthop==nb->nb_addr)&&(nb->nb_addr==nexthoop) )
						{
							Nodevisited=true;
							break;
						}
						if(((rt->rt_dst == antL->antL_NoeudEtabRout)&&(rt->rt_nexthop==nb->nb_addr))&&((rt->rt_seqno>seq)||((rt->rt_seqno==seq)&&(rt->rt_hops<d))))
						{			   			antL->antL_hop_count=rt->rt_hops ;
							antL->antL_src_seqno = rt->rt_seqno ;
							antL->antL_validPrevNER =true ;	
							nexthop=rt->rt_nexthop;
							dst=rt->rt_dst;
							valider=true;
						}
					}
		  			if (Nodevisited) break;		  	    
				}
				if((!valider)||(Nodevisited)) antL->antL_validPrevNER =false ;	
				if(antL->antL_validPrevNER==true)
				{
					antconf_rt_entry *ln= rtable.rt_lookup(nexthop,dst);
					if(ln) ln->pc_insert(nexthoop);
				}
			}
		}
		else if ((index !=antL->scr)&&(NotIntablePh))
		{
			//fprintf(stderr, " %s: sssssssssssssssssssssssssssssssssssssssssssssssssss\n",  __FUNCTION__);
			/*u_int32_t seq=0;
			bool Nodevisited=false;
			bool valider=false;
			u_int16_t d=10000;
			nsaddr_t dst;
			antL_nodetarget ==index)&& (antL->antL_rt == 1)) fprintf(stder		
			antconf_rt_entry *rt;
			ANTCONF_Neighbor *nb = nbhead.lh_first;
			for(; nb; nb = nb->nb_link.le_next) 
			{       rt  = rtable.rthead.lh_first;
				for(; rt; rt = rt->rt_link.le_next) 
				{
					if ((rt->rt_dst == antL->antL_NoeudEtabRout)&&(rt->rt_nexthop==nb->nb_addr)&&(nb->nb_addr==nexthoop) )
					{
						Nodevisited=true;
						break;
					}
					if(((rt->rt_dst == antL->antL_NoeudEtabRout)&&(rt->rt_nexthop==nb->nb_addr))&&((rt->rt_seqno>seq)||((rt->rt_seqno==seq)&&(rt->rt_hops<d))))
					{			   			  
						antL->antL_hop_count=rt->rt_hops ;
						antL->antL_src_seqno = rt->rt_seqno ;
						antL->antL_validPrevNER =true ;	
						nexthop=rt->rt_nexthop;
						dst=rt->rt_dst;
						valider=true;
					}
				}
		  		if (Nodevisited) break;		  	    
			}
			if((!valider)||(Nodevisited)) antL->antL_validPrevNER =false ;	*/
			//if(antL->antL_validPrevNER==true)
			//{
				//fprintf(stderr, " %s: sssssssssssssssssssssssssssssssssssssssssssssssssss\n",  __FUNCTION__);
					antconf_rt_entry *ln= rtable.rt_lookup(antL->antL_prevnode,antL->antL_NoeudEtabRout);
					if (ln) ln->pc_insert(nexthoop);
			//}
		}	 
        }
        if(nexthoop==-12) 
        {
        	sendAntLightReturn(antL->NodeVisitedCount,antL->NodeVisited,antL->scr, -1);
		Packet::free(p);
              	return;
        }	
     				 
	//if ((antL->antL_rt==0)&&(antL->antL_nodetarget!=-1)) fprintf(stderr, " %s:je suis %d sssssssssssssssssssssssssssssssssssssssssssssssssss %d dst %d par %d\n",  __FUNCTION__,index,antL->antL_nodetarget,antL->antL_NoeudEtabRout,nexthoop);
	antL->antL_prevnode=index; 
	antL->NodeVisited[antL->NodeVisitedCount] = index;
	antL->NodeVisitedCount += 1;
	antL->antL_hop_count += 1;
        antL->antL_nexthop=nexthoop;
	ch->next_hop_ =antL->antL_nexthop;
        ch->addr_type() = NS_AF_INET;
        ch->direction() = hdr_cmn::DOWN;
        if(antL->antL_nodetarget!=-1) ih->daddr() = antL->antL_nodetarget;
        Scheduler::instance().schedule(target_, p, DELAY);

}


void
ANTCONF::sendAntLightClone(Packet *p) 
{
	 Packet *pClone = Packet::alloc();
         struct hdr_cmn *chClone = HDR_CMN(pClone);
	 struct hdr_ip *ihClone = HDR_IP(pClone);
	 struct hdr_antconf_antlight *antLClone = HDR_ANTCONF_ANTLIGHT(pClone);

         struct hdr_cmn *ch = HDR_CMN(p);
	 struct hdr_ip *ih = HDR_IP(p);
	 struct hdr_antconf_antlight *antL = HDR_ANTCONF_ANTLIGHT(p);
	 
         ihClone->ttl_ =ih->ttl_;
	 nsaddr_t nexthp=-1;
         int nb_v=0;
         int i=0;
	 	
         //les champs de l'agent leger
	 antLClone->al_type = antL->al_type;
         antLClone->antL_hop_count = antL->antL_hop_count+1;
	 antLClone->scr = antL->scr;
	 antLClone->antL_id = antL->antL_id;
 	 //seqno += 2;
	 //assert ((seqno%2) == 0);
       	 antLClone->antL_src_seqno=antL->antL_src_seqno;
         antLClone->antL_prevnode=index;
	 antLClone->antL_nodetarget=-1;
         antLClone->antL_detask=-1;
         antLClone->antL_NoeudEtabRout=antL->scr;
         antLClone->antL_validPrevNER=antL->antL_validPrevNER;
         antLClone->NodeVisitedCount = antL->NodeVisitedCount;
	 antLClone->antL_rt=antL->antL_rt;
	 antLClone->ph_qte=0;
         
	/*if(nb_num>0)
	 {
                nb_v=rand()%nb_num;
                if(nb_v==0) nb_v=1;
                ANTCONF_Neighbor *nb1 = nbhead.lh_first;
		for(; nb1; nb1 = nb1->nb_link.le_next) 
         	{
			i++;
			if((i==nb_v))
			{       if (((nb1->nb_addr)!=(ch->prev_hop_)))
				{
				   nexthp=nb1->nb_addr;
				   break;
				} 
			}
		}
		if(index==0) fprintf(stderr, "i am %d(%d) my next hopt is %d   at %.2f port %d\n", index,idAgent,nexthp, Scheduler::instance().clock(),RT_PORT);
	 }*/


	ANTCONF_Neighbor *nb1 = nbhead.lh_first;
	 for(; nb1; nb1 = nb1->nb_link.le_next) 
         {
		if ((nb1->nb_addr)!=(ch->prev_hop_))
		{
		 nexthp=nb1->nb_addr;
		 break;
		}
	 }/**/
	 if(nexthp==-1) 
	{
	    	//fprintf(stderr, " %s: sssssssssssssssssssssssssssssssssssssssssssssssssss num nb %d\n",  __FUNCTION__,nb_num);
		sendAntLightReturn(antL->NodeVisitedCount,antL->NodeVisited,antL->scr,-1);
		Packet::free(p);
	    	return;	
	}	
        for(int k=0;k<antL->NodeVisitedCount;k++)
        antLClone->NodeVisited[k]=antL->NodeVisited[k];

	antLClone->NodeVisited[antL->NodeVisitedCount] = index;
	antLClone->NodeVisitedCount += 1;
	antLClone->antL_nexthop = nexthp;
	chClone->ptype() = PT_ANTCONF;
	chClone->size() = IP_HDR_LEN + antL->size();
	chClone->iface() = -2;
	chClone->error() = 0;
	chClone->addr_type() = NS_AF_NONE;
	chClone->prev_hop_ = index;
    	chClone->next_hop_ = nexthp;
	chClone->direction() = hdr_cmn::DOWN; 
	//#ifdef SEND_ANTLIGHT
	   	//if (antL->scr==0) fprintf(stderr, "i am %d(%d) my next hopt is %d   at %.2f port %d from  %d ttl %d\n", index,idAgent,nexthp, Scheduler::instance().clock(),RT_PORT,ih->saddr(),ih->ttl_);
        //#endif	
	ihClone->saddr() = ih->saddr();
	ihClone->daddr() = nexthp;		 
	ihClone->sport() = RT_PORT;
	ihClone->dport() = RT_PORT;	
        Scheduler::instance().schedule(target_, pClone, 0);
}

//******************************retour d'un agent
void
ANTCONF::sendAntLightReturn(u_int8_t  NVCount,nsaddr_t  NV[60], nsaddr_t  scr, nsaddr_t etab_node) // pk 60 
{
  	Packet *p = Packet::alloc();
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
  	struct hdr_antconf_antlight *antL = HDR_ANTCONF_ANTLIGHT(p);

        antL->NodeVisitedCount=NVCount;
        antL->scr=scr;
        for(int k=0;k<NVCount;k++)
        antL->NodeVisited[k]=NV[k];
	//int dem_v=0;
	//int i=0;

	u_int8_t nc;
        nc=antL->NodeVisitedCount-1;

	#ifdef RETURN_ANTLIGHT
  	  	fprintf(stderr, "i am %d(%d) return antlight at %.2f, i return by %d to the source %d nb hops is %d \n", index,idAgent, Scheduler::instance().clock(), antL->NodeVisited[nc], antL->NodeVisited[0], nc);
	#endif
        if(nb_lookup(antL->NodeVisited[nc]))
  	{
		antL->al_type = ANTCONFTYPE_ANTLIGHTRETURN;
  		antL->antL_NoeudEtabRout=index;
  		antL->antL_prevnode=index;
  		antL->antL_validPrevNER=true;
  		antL->antL_hop_count = 1; 
  		seqno += 2;
  		assert ((seqno%2) == 0);
  		antL->antL_src_seqno=seqno;
  		if(askRt) 
		{    
			/*if(dem_num>0)
	 		{
                		dem_v=rand()%dem_num;
                		if(dem_v==0) dem_v=1;

				AskRoute *ar = ARhead.lh_first;
				for(; ar; ar = ar->arlink.le_next) 
				{
					i++;
					if(i==dem_v) 
					{
						antL->antL_detask=ar->dst;
						break;
					}
				}
			}*/
		 if(etab_node != -1 && ar_lookup(etab_node)){
			
			AskRoute *ar = ARhead.lh_first;
			for(; ar; ar = ar->arlink.le_next) 
			{    
				if(ar->dst == etab_node){
					antL->antL_detask=ar->dst;
					antL->ph_qte=ar->ph_qte;
					break;
				}
			}
                 }
		 else
		 {	AskRoute *ar = ARhead.lh_first;
			for(; ar; ar = ar->arlink.le_next) 
			{
				antL->antL_detask=ar->dst;
				antL->ph_qte=ar->ph_qte;
			}
			ar_delete(antL->antL_detask);
			ar_insert(antL->antL_detask,antL->ph_qte);
                  }
			//fprintf(stderr, " %s: i am %d(%d) je suis un demandeur, je demandeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee%d nb demmm %d\n",  __FUNCTION__, index,idAgent,antL->antL_detask, dem_num);
		}else
		{
			antL->antL_detask=-2;
		}
  		ch->ptype() = PT_ANTCONF;
  		ch->size() = IP_HDR_LEN + antL->size();
  		ch->iface() = -2;
  		ch->error() = 0;
  		ch->addr_type() = NS_AF_INET;
  		ch->prev_hop_ = index;          // ANTCONF hack
  		ch->direction() = hdr_cmn::DOWN;
  		ch->next_hop_ = antL->NodeVisited[nc]; 
 
  		antL->antL_nexthop = antL->NodeVisited[nc];
  		antL->NodeVisitedCount--; 

  		ih->saddr() = index;
  		ih->daddr() = antL->NodeVisited[0];
  		ih->sport() = RT_PORT;
  		ih->dport() = RT_PORT;
  		ih->ttl_ = NVCount;
 		Scheduler::instance().schedule(target_, p, 0.0);
	}else
	{
          	//fprintf(stderr, "notttttttttttttttttttttttttttttt %d\n",index);
		Packet::free(p);
          	return;
	}
}


//reception d'un agent dans sa phase retour
void
ANTCONF::recvAntLightReturn(Packet *p) 
{
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);
	struct hdr_antconf_antlight *antL = HDR_ANTCONF_ANTLIGHT(p);
	//if (antL->antL_detask!=-2)  
	//	Tcl::instance().evalf("forward_demande %d  ",index);
	u_int8_t nc=0;
        nc=antL->NodeVisitedCount-1;
 			
    	#ifdef DEBUG
		fprintf(stderr, "%d - %s: received a antlight return\n", index, __FUNCTION__);
    	#endif // DEBUG
 
        /* maj de la table de routage */ 
	if((antL->antL_validPrevNER==true)&&(nb_lookup(antL->antL_prevnode)))
	{
                if(ar_lookup(antL->antL_NoeudEtabRout)) 
		{
			ar_delete(antL->antL_NoeudEtabRout);
			if(ar_empty()) askRt=false;
		}
		antconf_rt_entry *rt0; 
		// antL0 est la route vers NER
		rt0 = rtable.rt_lookup(antL->antL_prevnode, antL->antL_NoeudEtabRout);
		/* si la route n'existe pas cr?r une*/
		if(rt0 == 0) 
		{ 
			#ifdef ROUTE_TABLE_OPERATION
				fprintf(stderr, " %s: i am %d i am creating route:nexthop is  %d    destination: %d hopcount: %d  at % 2f\n",  __FUNCTION__, index,antL->antL_prevnode, antL->antL_NoeudEtabRout,antL->antL_hop_count,Scheduler::instance().clock());
			#endif
                	rt0 = rtable.rt_add(antL->antL_prevnode, antL->antL_NoeudEtabRout);
		}
		if ( (antL->antL_src_seqno > rt0->rt_seqno ) ||((antL->antL_src_seqno == rt0->rt_seqno) &&  (antL->antL_hop_count <rt0->rt_hops)) ) 
		{
			rt_update(rt0, antL->antL_src_seqno, antL->antL_hop_count, max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE)));
			assert (rt0->rt_flags == RTF_UP);
			Packet *buffered_pkt;
			while ((buffered_pkt = rqueue.deque(rt0->rt_dst))) 
			{
				if (rt0 && (rt0->rt_flags == RTF_UP)) 
				{
					//fprintf(stderr, "%s: je suis %d je depile pour dest  %d intermidiaire %d\n", __FUNCTION__,index,rt0->rt_dst,rt0->rt_hops);
					assert(rt0->rt_hops != INFINITY2);
					forward(rt0, buffered_pkt, NO_DELAY);
				}
			}
		 }  
	}

    //mettre a jour la table de pheromone
	if (antL->antL_detask!=-2)
	{
		antconf_rt_entry *rt;
		rt = rtable.rt_lookupoptimal(antL->antL_detask);
		if (rt!=0)
		{
		       sendAntLightRt(1,antL->antL_NoeudEtabRout,rt->rt_dst,antL->antL_prevnode,rt->rt_hops); 

		}else
		{
			//fprintf(stderr, "%d - %s: received a antlight return  j'ai la nonnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn\n", index, __FUNCTION__);
		}			
		antconf_phtable_entry *rtph0;
		rtph0 = phtable.rtph_lookup(antL->antL_NoeudEtabRout, antL->antL_detask);
		if (rtph0==0)
		{			
			rtph0=phtable.rtph_add(antL->antL_NoeudEtabRout, antL->antL_detask,antL->ph_qte);
			rtph0->nbph_insert(antL->antL_prevnode);
			//#ifdef PHEROMONE_TABLE_OPERATION
				//fprintf(stderr, "- %s: i am %d  i come from %d route pheromone insert: demandeur: %d destination demandée: %d pheromone quantitée: %2f neighbord: %d source: %d hop count: %d at %2f\n", __FUNCTION__, index, antL->antL_prevnode, antL->antL_NoeudEtabRout, antL->antL_detask, rtph0->rph_qte, antL->antL_prevnode, antL->scr, antL->antL_hop_count, Scheduler::instance().clock());
			//#endif
		}
		else
		{
			//fprintf(stderr, "- %s: i am %d  i come from %d route pheromone insert: demandeur: %d destination demandée: %d pheromone quantitée: %2f neighbord: %d source: %d hop count: %d at %2f\n", __FUNCTION__, index, antL->antL_prevnode, antL->antL_NoeudEtabRout, antL->antL_detask, rtph0->rph_qte, antL->antL_prevnode, antL->scr, antL->antL_hop_count, Scheduler::instance().clock());
			phtable.pheromone_add(antL->antL_NoeudEtabRout, antL->antL_detask,antL->ph_qte);
			rtph0->nbph_insert(antL->antL_prevnode);  
		}
	}
	//si c'est  moi qui as envoyé cet agent
	if((antL->scr==index))
	{
		//fprintf(stderr, "moi %d tableau %d source %d ttl %d\n", index, antL->NodeVisited[0],antL->scr,ih->ttl_);
		Packet::free(p);
		return;
	}
         
	//if((!valider)||(Nodevisited)) antL->antL_validPrevNER =false ;
	if((antL->antL_validPrevNER==true)&&(nb_lookup(antL->NodeVisited[nc])))
	{
		antconf_rt_entry *ln= rtable.rt_lookup(antL->antL_prevnode,antL->antL_NoeudEtabRout);
		if(ln) ln->pc_insert(antL->NodeVisited[nc]);
	 	antL->antL_hop_count += 1;        
	 	ch->next_hop_ = antL->NodeVisited[nc];  
         	antL->antL_nexthop = antL->NodeVisited[nc];      
         	antL->NodeVisitedCount--; 
         	ch->addr_type() = NS_AF_INET;
		ch->prev_hop_ = index;
         	ch->direction() = hdr_cmn::DOWN;//important: change the packet's direction
         	ih->daddr() = antL->NodeVisited[0];
		antL->antL_prevnode=index; 
		Scheduler::instance().schedule(target_, p, 0.);
	}else
	{
		Packet::free(p);
          	return;		
	}

}

//envoie d'un agent léger
void
ANTCONF::sendAntLightRt(bool rt, nsaddr_t  dem,nsaddr_t  dst,nsaddr_t  nxt,nsaddr_t  hc) 
{
	 Packet *p = Packet::alloc();
         struct hdr_cmn *ch = HDR_CMN(p);
	 struct hdr_ip *ih = HDR_IP(p);
	 struct hdr_antconf_antlight *antL = HDR_ANTCONF_ANTLIGHT(p);
	 if(rt)
 	 {
         	antL->al_type = ANTCONFTYPE_ANTLIGHTGO;
         	antL->antL_hop_count = hc;
	 	antL->scr = index;	
	 	antL->antL_id = idAgent++;	

	 	seqno += 2;
	 	assert ((seqno%2) == 0);

         	antL->antL_src_seqno=seqno;
         	antL->antL_prevnode=index;
	 	antL->antL_nodetarget=dem;
         	antL->antL_detask=dst;
         	antL->antL_NoeudEtabRout=dst;
         	antL->antL_validPrevNER=true;
	 	antL->NodeVisitedCount = 0;
	 	antL->antL_rt=1;
		
		antL->NodeVisited[antL->NodeVisitedCount] = index;
		antL->NodeVisitedCount += 1;
		antL->antL_nexthop = nxt;

		ch->ptype() = PT_ANTCONF;
		ch->size() = IP_HDR_LEN + antL->size();
		ch->iface() = -2;
		ch->error() = 0;
		ch->addr_type() = NS_AF_NONE;
		ch->prev_hop_ = index;
    		ch->next_hop_ = nxt;
		ch->direction() = hdr_cmn::DOWN; 

		#ifdef SEND_ANTLIGHT
	   		fprintf(stderr, "i am %d(%d) my next hopt is %d   at %.2f port %d\n", index,idAgent,nexthp, Scheduler::instance().clock(),RT_PORT);
        	#endif
	
		ih->saddr() = index;
		ih->daddr() = dem;
		//ih->ttl_=
		
	 }
	ih->sport() = RT_PORT;
	ih->dport() = RT_PORT;	
        Scheduler::instance().schedule(target_, p, 0.);
}


//verifier si le noeud a deja été visité
bool
ANTCONF::visitedNode(nsaddr_t  nb, nsaddr_t  NV[10],u_int8_t  NVCount)
{
        for(int k=0;k<NVCount;k++)
	{
        	if (NV[k]==nb) return true;	
	}
	return false;
}


//reception d'un agent rectificateur
void
ANTCONF::recvRectificatorAgent(Packet *p) 
{
   struct hdr_ip *ih = HDR_IP(p);
   struct hdr_antconf_rectificator *re = HDR_ANTCONF_RECTIFICATOR(p);
   antconf_rt_entry *rt;
   u_int8_t i;
   Packet *rerr = Packet::alloc();
   struct hdr_antconf_rectificator *nre = HDR_ANTCONF_RECTIFICATOR(rerr);
   nre->DestCount = 0;
   for (i=0; i<re->DestCount; i++) 
   {
      // For each unreachable destination
      rt = rtable.rt_lookup(re->unreachable_dst[i]);
      if ( rt && (rt->rt_hops != INFINITY2) && (rt->rt_nexthop == ih->saddr()) && (rt->rt_seqno <= re->unreachable_dst_seqno[i]) ) 
	  {
	    assert(rt->rt_flags == RTF_UP);
	    assert((rt->rt_seqno%2) == 0); // is the seqno even?
        
     	rt->rt_seqno = re->unreachable_dst_seqno[i];
     	rt_down(rt);
        // Not sure whether this is the right thing to do
        Packet *pkt;
	while((pkt = ifqueue->filter(ih->saddr()))) 
	{
        	//fprintf(stderr, "%s: calling drop() rectificator for %d ih->saddr()\n", __FUNCTION__);
		drop(pkt, DROP_RTR_MAC_CALLBACK);
     	}
        // if precursor list non-empty add to RERR and delete the precursor list
     	if (!rt->pc_empty()) 
	{
     		nre->unreachable_dst[nre->DestCount] = rt->rt_dst;
     		nre->unreachable_dst_seqno[nre->DestCount] = rt->rt_seqno;
     		nre->DestCount += 1;
		rt->pc_delete();
     	}
      }
   }
   if (nre->DestCount > 0) 
   {
       #ifdef ERROR
          fprintf(stderr, "%s(%f): %d\t forwarding Rectificator Agent at %2f...\n", __FUNCTION__, CURRENT_TIME, index,Scheduler::instance().clock());
       #endif // DEBUG
       sendRectificatorAgent(rerr);
   }
   else 
   {
     Packet::free(rerr);
   }
   Packet::free(p);
}
/*
   gestion de l'envoie des paquets
*/


//envoie d'un agent rectificateur
void
ANTCONF::sendRectificatorAgent(Packet *p, bool jitter) 
{
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_antconf_rectificator *re = HDR_ANTCONF_RECTIFICATOR(p);
  
  re->re_type = ANTCONFTYPE_RECTIFICATOR;
  
  ch->ptype() = PT_ANTCONF;
  ch->size() = IP_HDR_LEN + re->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->next_hop_ = 0;
  ch->prev_hop_ = index;          // ANTCONF hack
  ch->direction() = hdr_cmn::DOWN;       //important: change the packet's direction 
  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = 1;
  // jitter pour evité les collusions
  if (jitter)
 	Scheduler::instance().schedule(target_, p, 0.01*Random::uniform());
  else
 	Scheduler::instance().schedule(target_, p, 0.0);

}

/*
   gestion des voisins
*/
void
ANTCONF::sendHello() 
{
  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_antconf_hello *rh = HDR_ANTCONF_HELLO(p);

  #ifdef LOCAL_CONNECTIVITY
    fprintf(stderr, "i am %d, I sending Hello  at %.2f\n", index, Scheduler::instance().clock());
  #endif 

  rh->hl_type = ANTCONFTYPE_HELLO;
  rh->hl_hop_count = 1;
  rh->hl_dst = index;
  rh->hl_dst_seqno = seqno;
  

  ch->ptype() = PT_ANTCONF;
  ch->size() = IP_HDR_LEN + rh->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->prev_hop_ = index;          // ANTCONF hack

  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = 1;
  Scheduler::instance().schedule(target_, p, 0.0);
}


//reception d'un message hello
void
ANTCONF::recvHello(Packet *p) 
{
  struct hdr_antconf_hello *rh = HDR_ANTCONF_HELLO(p);
   #ifdef LOCAL_CONNECTIVITY
      fprintf(stderr, "i am %d, I reciving Hello from %d at %.2f\n",index, rh->hl_dst, Scheduler::instance().clock());
  #endif 
  ANTCONF_Neighbor *nb;
  nb = nb_lookup(rh->hl_dst);
  if(nb == 0) 
  {
     nb_insert(rh->hl_dst);
  }
  else 
  {
      nb->nb_expire = CURRENT_TIME + (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
  }
  Packet::free(p);
}

//inserer un voisin
void
ANTCONF::nb_insert(nsaddr_t id) 
{
  ANTCONF_Neighbor *nb = new ANTCONF_Neighbor(id);
  assert(nb);
  nb->nb_expire = CURRENT_TIME +(1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
  LIST_INSERT_HEAD(&nbhead, nb, nb_link);
  seqno += 2;             
  assert ((seqno%2) == 0);
  nb_num++;
}

//verifier l'existance d'un voisin
ANTCONF_Neighbor*
ANTCONF::nb_lookup(nsaddr_t id) 
{
  ANTCONF_Neighbor *nb = nbhead.lh_first;
  for(; nb; nb = nb->nb_link.le_next) 
  {
     if(nb->nb_addr == id) break;
  }
  return nb;
}


//supprimer un voisin
void
ANTCONF::nb_delete(nsaddr_t id) 
{
	ANTCONF_Neighbor *nb = nbhead.lh_first;
	log_link_del(id);
	seqno += 2;     
	assert ((seqno%2) == 0);
	for(; nb; nb = nb->nb_link.le_next) 
	{
		if(nb->nb_addr == id) 
		{
			//#ifdef ERROR
			 //fprintf(stderr, "i am %d i deliting neighbord %d at %.2f\n",index, nb->nb_addr, Scheduler::instance().clock());
			//#endif
			LIST_REMOVE(nb,nb_link);
			delete nb;
			nb_num--;
			break;
		}
	}
	link_failure_management(id);
}


// purger la table des voisins  
void
ANTCONF::nb_purge() 
{
  ANTCONF_Neighbor *nb = nbhead.lh_first;

  ANTCONF_Neighbor *nbn;
  double now = CURRENT_TIME;

  for(; nb; nb = nbn) 
  { 
     nbn = nb->nb_link.le_next;
     if(nb->nb_expire <= now) 
     {
       
       nb_delete(nb->nb_addr);
     }
  }
 
}


//**************************purger les demandes de routes***********************************//
void
ANTCONF::ar_purge() 
{
   
  AskRoute *ar = ARhead.lh_first;
  AskRoute *arn;
  double now = CURRENT_TIME;
  for(; ar; ar = arn) 
  {
     arn = ar->arlink.le_next;
     if(ar->expire <= now) 
     {
	//fprintf(stderr, "%s: ia am %d deleting paquet agent for dest %d purrge at %2f...\n", __FUNCTION__,index,ar->dst,Scheduler::instance().clock());

	ar_delete(ar->dst);
     }
  }
 
}


/*********************************inserer une demande de route*********************************************/
void
ANTCONF::ar_insert(nsaddr_t id,double ph_qte) 
{
 
  AskRoute *ar = new AskRoute(id);
  assert(ar);
  ar->expire = CURRENT_TIME +EXPIRE_AR_INTERVAL;
  if (ph_qte==0)
	ar->ph_qte=PHEROMONE_DEMANDE_QTE;
	else ar->ph_qte=ph_qte;
  LIST_INSERT_HEAD(&ARhead, ar, arlink);
  dem_num++;
}

/*********************************verifier si la demande existe*********************************************/
bool
ANTCONF::ar_lookup(nsaddr_t id) 
{
  AskRoute *ar = ARhead.lh_first; 
  for( ; ar; ar = ar->arlink.le_next) 
  {
   if (ar->dst == id)   return true;     
  }
 return false;
}

/*********************************supprimer une demande de route*********************************************/
void
ANTCONF::ar_delete(nsaddr_t id) 
{	
	AskRoute *ar = ARhead.lh_first;
	for(; ar; ar = ar->arlink.le_next) 
	{
		if(ar->dst == id) 
		{
			LIST_REMOVE(ar,arlink);
			delete ar;
                        dem_num--;
			break;
		}
	}
	if (ar_empty())
	{
		askRt=false;
	}
}

//purger la table des demandes
void
ANTCONF::phd_purge()
{   
	AskRoute *ar = ARhead.lh_first;
	for(; ar; ar = ar->arlink.le_next) 
	{
		ar->ph_qte+=INC_PHEROMONE;
	}
}

/*********************************verifier si vide*********************************************/
bool
ANTCONF::ar_empty(void) 
{
	 /*AskRoute *ar;
	 if ((ar == ARhead.lh_first)) return false;
	 else return true;*/
	 int i=0;
	 AskRoute *ar = ARhead.lh_first;
	 for(; ar; ar = ar->arlink.le_next) 
         {
		i++;
	 }
          if (i==0) return true;
	 else return false;
}	


/*********************************renvoi une entrée*********************************************/
/*nsaddr_t
ANTCONF::ar_getlaste(nsaddr_t id)
{
   	AskRoute *ar = ARhead.lh_first;
	for(; ar; ar = ar->arlink.le_next) 
	{
		if(ar->dst == id) 
		{
			LIST_REMOVE(ar,arlink);
			delete ar;
                        dem_num--;
			break;
		}
	}
	return ARhead.lh_first->dst;
}*/
