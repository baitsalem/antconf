// authors:
// AIT SALEM Boussad
//  Riahla Amine

#include <antconf/antconf_phtable.h>
/*
  La table de pheromone
*/

antconf_phtable_entry::antconf_phtable_entry()
{
	rph_dsttoask = 0;
	rph_claimant=0;
	rph_qte=PHEROMONE_QTE;		
	LIST_INIT(&rtph_nblist);	 
}


antconf_phtable_entry::~antconf_phtable_entry()
{
	ANTCONF_NeighborPh *nbph;
	while((nbph = rtph_nblist.lh_first)) 
	{
		 LIST_REMOVE(nbph, nbph_link);
		 delete nbph;
	}	
}

void
antconf_phtable_entry::nbph_insert(nsaddr_t id)
{
	if (nbph_lookup(id) == NULL) 
	{
		ANTCONF_NeighborPh *nbph = new ANTCONF_NeighborPh(id);
        assert(nbph);
 		LIST_INSERT_HEAD(&rtph_nblist, nbph, nbph_link);
	}
}


ANTCONF_NeighborPh*
antconf_phtable_entry::nbph_lookup(nsaddr_t id)
{
	ANTCONF_NeighborPh *nbph = rtph_nblist.lh_first;
	for(; nbph; nbph = nbph->nbph_link.le_next) 
	{
	if(nbph->nbph_addr == id)
		return nbph;
	}
	return NULL;
}

void
antconf_phtable_entry::nbph_delete(nsaddr_t id) 
{
	ANTCONF_NeighborPh *nbph = rtph_nblist.lh_first;
	for(; nbph; nbph = nbph->nbph_link.le_next) 
	{
		if(nbph->nbph_addr == id) 
		{	
			LIST_REMOVE(nbph,nbph_link);
			delete nbph;
			break;
		}
	}

}


antconf_phtable_entry*
antconf_phtable::rtph_lookup(nsaddr_t claimant, nsaddr_t dstask)
{
	antconf_phtable_entry *rt = phthead.lh_first;
	for(; rt; rt = rt->pht_link.le_next) 
	{
		if((rt->rph_claimant == claimant)&&(rt->rph_dsttoask == dstask))
		break;
	}
	 return rt;
}

void
antconf_phtable::rtph_delete(nsaddr_t claimant, nsaddr_t dstask)
{
	antconf_phtable_entry *rt = rtph_lookup(claimant, dstask);
	if(rt) 
	{	
		LIST_REMOVE(rt, pht_link);
		delete rt;
	}

}

antconf_phtable_entry*
antconf_phtable::rtph_add(nsaddr_t claimant, nsaddr_t dstask,double ph_qte)
{
	antconf_phtable_entry *rt;
	assert(rtph_lookup(id) == 0);
	rt = new antconf_phtable_entry;
	if(ph_qte!=0) rt->rph_qte=ph_qte;
	assert(rt);
	rt->rph_claimant = claimant;
    	rt->rph_dsttoask = dstask;
	//rt->rph_qte
	LIST_INSERT_HEAD(&phthead, rt, pht_link);
	return rt;
}

void
antconf_phtable::pheromone_add(nsaddr_t claimant, nsaddr_t dstask,double ph_qte)
{
	antconf_phtable_entry *rt = phthead.lh_first;
	for(; rt; rt = rt->pht_link.le_next) 
	{
		if((rt->rph_claimant == claimant)&&(rt->rph_dsttoask == dstask))
		{
		  //rt->rph_qte+=PHEROMONE_QTE_ADD;
		  if(ph_qte!=0) rt->rph_qte+=ph_qte;
		   break;
		}
	}	 
}
