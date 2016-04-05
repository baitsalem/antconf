
#ifndef __antconf_h__
#define __antconf_h__

#include <cmu-trace.h>
#include <priqueue.h>
#include <antconf/antconf_rtable.h>
#include <antconf/antconf_phtable.h>
#include <antconf/antconf_rqueue.h>
#include <classifier/classifier-port.h>



/*
  permettre d'utiliser la connectivité locale (hello messages,...)
*/
#define ANTCONF_LINK_LAYER_DETECTION





class ANTCONF;

//#define MY_ROUTE_TIMEOUT        10                      	// 
#define ACTIVE_ROUTE_TIMEOUT    10				// 
#define REV_ROUTE_LIFE          10				// 
#define AGENT_ID_SAVE           6				// 


// diametre du reseau choisit selon la dimention du reseau
#define NETWORK_DIAMETER        30




//#define ID_NOT_FOUND    0x00
//#define ID_FOUND        0x01
//#define INFINITY       0xff 


#define DELAY 1.0 
#define NO_DELAY -1.0 


#define ARP_DELAY 0.01

//quantité de pheromone a decrementé
#define DEC_PHEROMONE 0.1

//quantité de pheromone a incrementé pour les demandes
#define INC_PHEROMONE 0.1 //0.1

// quantitée de pheromone initiale des demandes
#define PHEROMONE_DEMANDE_QTE 0.5      

//intervall pour purger la table des demandes de route
#define EXPIRE_AR_INTERVAL          10               // 1000 ms

//intervall des mesages hello
#define HELLO_INTERVAL          1               // 1000 ms

//nombre max de messages hello non recu pour supprimer le voisin
#define ALLOWED_HELLO_LOSS      3               
#define MaxHelloInterval        (2 * HELLO_INTERVAL)
#define MinHelloInterval        (1 * HELLO_INTERVAL)

//intervall d'envoie des agents
#define AntSendInterval 0.8// 0,8
#define MinAntSendInterval (0.75 * AntSendInterval)
#define MaxAntSendInterval (1.25 * AntSendInterval)


//***************************************les timers*****************************//
class AgentIdTimer : public Handler 
{
public:
        AgentIdTimer(ANTCONF* a) : agent(a) {}
        void	handle(Event*);
private:
        ANTCONF    *agent;
		Event	intr;
};

class HelloTimerAnt : public Handler 
{
public:
        HelloTimerAnt(ANTCONF* a) : agent(a) {}
        void	handle(Event*);
private:
        ANTCONF    *agent;
		Event	intr;
};

class NeighborTimerAnt : public Handler 
{
public:
        NeighborTimerAnt(ANTCONF* a) : agent(a) {}
        void	handle(Event*);
private:
        ANTCONF    *agent;
		Event	intr;
};

class RouteCacheTimerAnt : public Handler 
{
public:
        RouteCacheTimerAnt(ANTCONF* a) : agent(a) {}
        void	handle(Event*);
private:
        ANTCONF    *agent;
		Event	intr;
};

class LightTimerAnt : public Handler 
{
public:
        LightTimerAnt(ANTCONF* a) : agent(a) {}
        void	handle(Event*);
private:
        ANTCONF    *agent;
		Event	intr;
};

class PheromoneDemandeTableTimer : public Handler 
{
public:
        PheromoneDemandeTableTimer(ANTCONF* a) : agent(a) {}
        void	handle(Event*);
private:
        ANTCONF    *agent;
		Event	intr;
};

class PheromoneTableTimer : public Handler 
{
public:
        PheromoneTableTimer(ANTCONF* a) : agent(a) {}
        void	handle(Event*);
private:
        ANTCONF    *agent;
		Event	intr;
};

class LocalRepairTimerAnt : public Handler 
{
public:
        LocalRepairTimerAnt(ANTCONF* a) : agent(a) {}
        void	handle(Event*);
private:
        ANTCONF    *agent;
		Event	intr;
};

class RouteAskTimer : public Handler 
{
public:
        RouteAskTimer(ANTCONF* a) : agent(a) {}
        void	handle(Event*);
private:
        ANTCONF    *agent;
		Event	intr;
};

/*
  gestion des listes d'agents passés par ce noeud
*/
class AgentID 
{
        friend class ANTCONF;
 public:
        AgentID(nsaddr_t i, u_int32_t b) { src = i; id = b;  }
 protected:
        LIST_ENTRY(AgentID) link;
        nsaddr_t        src;
        u_int32_t       id;
        double          expire;
};
LIST_HEAD(antconf_bcache, AgentID);


/*
  gestion des demandes de routes
*/
class AskRoute 
{
        friend class ANTCONF;
 public:
        AskRoute(nsaddr_t i) { dst = i;}
 protected:
        LIST_ENTRY(AskRoute) arlink;
        nsaddr_t        dst;
        double          expire;
	double          ph_qte;
};
LIST_HEAD(antconf_AskRoutecache, AskRoute);


/*
   ANTCONF
*/
class ANTCONF: public Agent 
{
        //les classes amies de antconf
        friend class antconf_rt_entry;
        friend class AgentIdTimer;
        friend class HelloTimerAnt;
        friend class NeighborTimerAnt;
	friend class LightTimerAnt;
        friend class RouteCacheTimerAnt;
        friend class LocalRepairTimerAnt; 
        friend class PheromoneTableTimer;
	friend class PheromoneDemandeTableTimer;
        friend class RouteAskTimer;

 public:
        ANTCONF(nsaddr_t id);
       
	void      	recv(Packet *p, Handler*);
		
 protected:
        
        int             command(int, const char *const *);
        int             initialized() { return 1 && target_; }
        int                 nb_num;
	int                 dem_num;
        
/*****************************gestion de la table de routage****************************/ 
        void            rt_resolution(Packet *p);
        void            rt_update(antconf_rt_entry *rt, u_int32_t seqnum, u_int16_t metric,double expire_time);        
        void            rt_down(antconf_rt_entry *rt);
        void            rt_repair(antconf_rt_entry *rt, Packet *p);

        public:
        void            linklayer_failed(Packet *p);
        void            link_failure_management(nsaddr_t id);

 protected:         
        void            rt_purge(void);
        void            enque(antconf_rt_entry *rt, Packet *p);
        Packet*         deque(antconf_rt_entry *rt);



/***************************gestion de la table des voisins**************************/
        void                               nb_insert(nsaddr_t id);
        ANTCONF_Neighbor*                  nb_lookup(nsaddr_t id);
        void                               nb_delete(nsaddr_t id);
        void                               nb_purge(void);

/***************************gestion de la table des pheromones***********************/
	void                               pht_purge(void);


/***************************gestion de la table des demandes***********************/
	void                               phd_purge(void);


/***************************gestion de la table des id agents*************************/
        void            id_insert(nsaddr_t id, u_int32_t bid);
        bool		id_lookup(nsaddr_t id, u_int32_t bid);
        void            id_purge(void);

/***************************gestion des demandes de route*****************************/
	void            ar_purge(void);
        void            ar_insert(nsaddr_t id,double ph_qte);
        bool            ar_lookup(nsaddr_t id);
        void            ar_delete(nsaddr_t id);
	nsaddr_t        ar_getfirst(nsaddr_t id);
	bool 		ar_empty(void);



/******************************gestion des paquets: envoie*********************************/
        void            forward(antconf_rt_entry *rt, Packet *p, double delay);
      
        void            sendHello(void);
       
        void            sendAntLight(); 
	void            sendAntLightClone(Packet *p);
	void            sendAntLightRt(bool rt, nsaddr_t  dem,nsaddr_t  dst,nsaddr_t  nxt, nsaddr_t  hc);
       
        void            sendAntLightReturn(u_int8_t  NVCount,nsaddr_t  NV[10], nsaddr_t  scr, nsaddr_t etab_node);
       
        void            sendRectificatorAgent(Packet *p, bool jitter = true);

		 
/*****************************gestion des paquets: reception*********************************/
        void            recvANTCONF(Packet *p);
     
        void            recvHello(Packet *p); 
 
        void            recvAntLightGo(Packet *p); 
      
        void            recvAntLightReturn(Packet *p);
    
	void            recvRectificatorAgent(Packet *p);
        
        void            chooseNextHop(Packet *p);
	
        bool		visitedNode(nsaddr_t  nb, nsaddr_t  NV[10],u_int8_t  NVCount);
	
/**************************les attributs**************************************************/       
        nsaddr_t            index;                  // Adresse IP du noeud
        u_int32_t           seqno;                  // numéros de Sequence 
        int                 idAgent;                // Agent ID
        bool                askRt;                  //si le noeud demande une route ou non



        
        //antconf_rtable					rthead;           // table de routage 
        antconf_ncache					nbhead;           // table des voisins
        antconf_bcache					bihead;           // AgentID Table
        antconf_AskRoutecache      ARhead;                 //liste des demandes de route

 /**************************************Timers***********************************************/
	LightTimerAnt			lttimer;       
	AgentIdTimer			btimer;        
	HelloTimerAnt       		htimer;    
	NeighborTimerAnt   		ntimer;
	RouteCacheTimerAnt 		rtimer;        
	PheromoneTableTimer 		ptimer;	
	PheromoneDemandeTableTimer 	dtimer;	
	LocalRepairTimerAnt 		lrtimer;       
	RouteAskTimer 			ratimer;
		
	//Table de routage
        antconf_rtable          rtable;

	//Table de pheromone
	antconf_phtable         phtable;

        //la file d'attente utilisée par le protocole antconf a fin de sauvegarder les paquets qui n'ont pas de destination(tres interessante)
        antconf_rqueue          rqueue;

        
        Trace           *logtarget;


	//pointeur vers la file d'attente entre le classifier et la couche liaison de donnée  
        PriQueue        *ifqueue;

        //journalisation des liens
        void            log_link_del(nsaddr_t dst);
        void            log_link_broke(Packet *p);
        void            log_link_kept(nsaddr_t dst);

	/* pour envoyé le paquet a l'agent suivant */
	PortClassifier *dmux_;

};

#endif 
