#ifndef __antconf_phrtable_h__
#define __antconf_phrtable_h__

#include <assert.h>
#include <sys/types.h>
#include <config.h>
#include <lib/bsd-list.h>
#include <scheduler.h>

#define CURRENT_TIME    Scheduler::instance().clock()
#define INFINITY2   0xff

#define PHEROMONE_QTE 0.5      // quantitée de pheromone initiale
#define PHEROMONE_QTE_ADD  0.1      // quantitée de pheromone a ajouté
//#define DEC_PHEROMONE  0.1      // quantit�de ph�omone a soustraire


/*
   listeV
*/
class ANTCONF_NeighborPh 
{
        friend class ANTCONF;
        friend class antconf_phtable_entry;
 public:
        ANTCONF_NeighborPh(u_int32_t a) { nbph_addr = a;  }

 protected:
        LIST_ENTRY(ANTCONF_NeighborPh) nbph_link;
        nsaddr_t        nbph_addr;
};

LIST_HEAD(antconf_nphcache, ANTCONF_NeighborPh);


/*
  entrée de la table de pheromone
*/

class antconf_phtable_entry 
{
        friend class antconf_phtable;
        friend class ANTCONF;
	friend class LocalRepairTimer;
 public:
        antconf_phtable_entry();
        ~antconf_phtable_entry();
		
	//gestion des  entrées de la table
        void                     nbph_insert(nsaddr_t id);
        ANTCONF_NeighborPh*      nbph_lookup(nsaddr_t id);
        void 		         nbph_delete(nsaddr_t id);
        void 			 nbph_delete(void);
        bool 			 nbph_empty(void);
        
 protected:
        LIST_ENTRY(antconf_phtable_entry) pht_link;
        nsaddr_t        rph_claimant;//demandeur
	nsaddr_t        rph_dsttoask;//destination demandée
        double          rph_qte;
       
	    /* Listv */ 
        antconf_nphcache rtph_nblist;
      
};
/*
  Table de pheromone
*/

class antconf_phtable 
{
 public:
    //constructeur
    antconf_phtable()               { LIST_INIT(&phthead); }
    //entrée de la table
    antconf_phtable_entry*       head() { return phthead.lh_first; }
    //ajout d'une entrée
    antconf_phtable_entry*       rtph_add(nsaddr_t claimant, nsaddr_t dstask, double ph_qte);
    //suppression d'une entrée
    void			 rtph_delete(nsaddr_t claimant, nsaddr_t dstask);
    //verifier si une entrée existe
    antconf_phtable_entry*       rtph_lookup(nsaddr_t claimant, nsaddr_t dstask);
    //ajout de pheromone
    void                     pheromone_add(nsaddr_t claimant, nsaddr_t dstask, double ph_qte);
    LIST_HEAD(antconf_phthead, antconf_phtable_entry) phthead;
};
#endif /* _antconf__rtable_h__ */
