// authors:
// AIT SALEM Boussad
//  Riahla Amine

#ifndef __antconf_rtable_h__
#define __antconf_rtable_h__

#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>

#define CURRENT_TIME    Scheduler::instance().clock()
#define INFINITY2        0xff

/*
   la classe des voisins du noeud 
*/
class ANTCONF_Neighbor 
{
        friend class ANTCONF;
        friend class antconf_rt_entry;
 public:
        ANTCONF_Neighbor (u_int32_t a) { nb_addr = a; }

 protected:
        LIST_ENTRY(ANTCONF_Neighbor) nb_link;
        nsaddr_t        nb_addr;
        double          nb_expire;      // ALLOWED_HELLO_LOSS * HELLO_INTERVAL
};

LIST_HEAD(antconf_ncache, ANTCONF_Neighbor);

/*
   la classe Precursor  Listv*/
class ANTCONF_Precursor 
{
        friend class ANTCONF;
        friend class antconf_rt_entry;
 public:
        ANTCONF_Precursor(u_int32_t a) { pc_addr = a; }

 protected:
        LIST_ENTRY(ANTCONF_Precursor) pc_link;
        nsaddr_t        pc_addr;	
};

LIST_HEAD(antconf_precursors, ANTCONF_Precursor);


/*
  entrée de la table de routage
*/

class antconf_rt_entry 
{
        friend class antconf_rtable;
        friend class ANTCONF;
	friend class LocalRepairTimerAnt;
 public:
        antconf_rt_entry();
        ~antconf_rt_entry();
        
        void            pc_insert(nsaddr_t id);
        ANTCONF_Precursor* pc_lookup(nsaddr_t id);
        void 		pc_delete(nsaddr_t id);
        void 		pc_delete(void);
        bool 		pc_empty(void);
       
 protected:
        LIST_ENTRY(antconf_rt_entry) rt_link;
        nsaddr_t        rt_dst;
        u_int32_t       rt_seqno;

	u_int16_t       rt_hops;       		// nombre de sauts
	   

        nsaddr_t        rt_nexthop;    		// prochaine saut

	    /* Listv */ 
        antconf_precursors rt_pclist;

        double          rt_expire;     		// date d'expiration de l'entrée
        u_int8_t        rt_flags;          //etat de l'entrée: bas, haut ou en reparation

       #define RTF_DOWN 0
       #define RTF_UP 1
       #define RTF_IN_REPAIR 2

};


/*
  la table de routage
*/

class antconf_rtable 
{
 public:
    antconf_rtable() { LIST_INIT(&rthead); }
    antconf_rt_entry*       head() { return rthead.lh_first; }
    antconf_rt_entry*       rt_add(nsaddr_t nexthopid, nsaddr_t id);
    antconf_rt_entry*       rt_add(nsaddr_t id);
    void                    rt_delete(nsaddr_t nexthopid, nsaddr_t id);
    antconf_rt_entry*       rt_lookupoptimal(nsaddr_t id);
    antconf_rt_entry*       rt_lookup(nsaddr_t id);
    antconf_rt_entry*       rt_lookup(nsaddr_t nexthopid, nsaddr_t id);
 
    LIST_HEAD(antconf_rthead, antconf_rt_entry) rthead;
};
#endif /* _antconf__rtable_h__ */
