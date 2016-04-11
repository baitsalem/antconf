// authors:
// AIT SALEM Boussad
//  Riahla Amine

#ifndef __antconf_packet_h__
#define __antconf_packet_h__



/* =====================================================================
   type des paquets/agents
   ===================================================================== */
#define ANTCONFTYPE_ANTLIGHTGO  	0x02
#define ANTCONFTYPE_ANTLIGHTRETURN   	0x04
#define ANTCONFTYPE_RECTIFICATOR   	0x08
#define ANTCONFTYPE_HELLO   	        0x01

#define ANTCONF_MAX_NODE_VISITED   	50
#define ANTCONF_MAX_ERRORS   	100








/*
 * pour recuperer les entètes de chaque paquet/agent
 */
#define HDR_ANTCONF(p)		((struct hdr_antconf*)hdr_antconf::access(p))
#define HDR_ANTCONF_ANTLIGHT(p)  	((struct hdr_antconf_antlight*)hdr_antconf::access(p))
#define HDR_ANTCONF_HELLO(p)	((struct hdr_antconf_hello*)hdr_antconf::access(p))
#define HDR_ANTCONF_RECTIFICATOR(p)	((struct hdr_antconf_rectificator*)hdr_antconf::access(p))


/*
 * entète partagé par tous les formats
 */
struct hdr_antconf 
{
    	u_int8_t        ah_type;	
	// méthodes d'accès aux entètes
	static int offset_; // exigé par la classe PacketHeaderManager
	inline static int& offset() 
	{ 
		return offset_; 
	}
	inline static hdr_antconf* access(const Packet* p) 
	{
	    return (hdr_antconf*) p->access(offset_);
	}
};




//entète de l'agent  antlight
struct hdr_antconf_antlight 
{
    u_int8_t        al_type;	// Type
    u_int32_t       antL_src_seqno;   // numeros de sequence
    u_int8_t        reserved[2];            //reserver
    nsaddr_t        antL_prevnode;         // noeud precedent
    nsaddr_t        antL_nodetarget;   //noeud cible
    nsaddr_t        antL_detask;   //destination demandée
    nsaddr_t        antL_NoeudEtabRout; //NER
    bool            antL_validPrevNER; //valide
    u_int8_t        antL_hop_count;   // nombre de sauts
    nsaddr_t        antL_nexthop;         // prochain saut	
    u_int32_t       antL_id;    //  ID de l'agent   
    nsaddr_t        scr; //source
    u_int8_t        NodeVisitedCount;                 // le nombre de noeuds visités
    nsaddr_t        NodeVisited[ANTCONF_MAX_NODE_VISITED];   //noeuds visités
    nsaddr_t        demandes[ANTCONF_MAX_NODE_VISITED];
    bool            antL_rt;
    double          ph_qte;
    
    inline int size() 
    { 
      int sz = 0;  	
  	    
	sz = 2*sizeof(u_int32_t)+4*sizeof(u_int8_t)+NodeVisitedCount*sizeof(nsaddr_t);
  	 
  	  assert (sz >= 0);
	  return sz;
    }
};


struct hdr_antconf_hello 
{
        u_int8_t         hl_type;        // Type
        u_int8_t         reserved[2];            //reserver
        u_int8_t         hl_hop_count;           // nombre de sauts
        nsaddr_t         hl_dst;                 //  Addresse IP Destination
        u_int32_t        hl_dst_seqno;           // Numeros de sequence Destination 
        nsaddr_t         hl_src;                 // IP Source 
        						
        inline int size() 
	{ 
          int sz = 0;
    	  sz = 6*sizeof(u_int32_t);
  	      assert (sz >= 0);
	      return sz;
        }

};


struct hdr_antconf_rectificator 
{
        u_int8_t        re_type;                // Type
        u_int8_t        reserved[2];            // reserver
        u_int8_t        DestCount;                
        nsaddr_t        unreachable_dst[ANTCONF_MAX_ERRORS];   
        u_int32_t       unreachable_dst_seqno[ANTCONF_MAX_ERRORS]; 	
  	

        inline int size() 
         { 
           int sz = 0;  
           sz = (DestCount*2 + 1)*sizeof(u_int32_t);
	       assert(sz);
           return sz;
          }

};

// pour calculer la taille de l'entète
union hdr_all_antconf 
{
  hdr_antconf          ah;
  hdr_antconf_antlight  antl;
  hdr_antconf_hello    rhel;
  hdr_antconf_rectificator    rerr;
 };

#endif /* __antconf_packet_h__ */
