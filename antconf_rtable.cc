// authors:
// AIT SALEM Boussad
//  Riahla Amine

#include <antconf/antconf_rtable.h>


/*
  Les entrées de La table de routage
*/

antconf_rt_entry::antconf_rt_entry()
{
   
	rt_dst = -1;
	rt_seqno = 0;
	rt_hops = INFINITY2;
	rt_nexthop = -1;
	LIST_INIT(&rt_pclist);
	rt_expire = 0.0;
	rt_flags = RTF_DOWN;	
	
}


antconf_rt_entry::~antconf_rt_entry()
{	
	 ANTCONF_Precursor *pc;
	 while((pc = rt_pclist.lh_first)) 
	{
		 LIST_REMOVE(pc, pc_link);
		delete pc;
	}
}

void
antconf_rt_entry::pc_insert(nsaddr_t id)
{
	if (pc_lookup(id) == NULL) 
	{
		ANTCONF_Precursor *pc = new ANTCONF_Precursor(id);
        	assert(pc);
 		LIST_INSERT_HEAD(&rt_pclist, pc, pc_link);
	}
}


ANTCONF_Precursor*
antconf_rt_entry::pc_lookup(nsaddr_t id)
{
	ANTCONF_Precursor *pc = rt_pclist.lh_first;
	for(; pc; pc = pc->pc_link.le_next) 
	{
	if(pc->pc_addr == id)
		return pc;
	}
	return NULL;
}

void
antconf_rt_entry::pc_delete(nsaddr_t id) 
{
	ANTCONF_Precursor *pc = rt_pclist.lh_first;
	for(; pc; pc = pc->pc_link.le_next) 
	{
		if(pc->pc_addr == id) 
		{
			LIST_REMOVE(pc,pc_link);
			delete pc;
			break;
		}
	}

}

void
antconf_rt_entry::pc_delete(void) 
{
	ANTCONF_Precursor *pc;
	while((pc = rt_pclist.lh_first)) 
	{
		LIST_REMOVE(pc, pc_link);
		delete pc;
	}
}	

bool
antconf_rt_entry::pc_empty(void) 
{
	ANTCONF_Precursor *pc;
	 if ((pc = rt_pclist.lh_first)) return false;
	else return true;
}	

/*
  La table de routage
*/
antconf_rt_entry*
antconf_rtable::rt_lookup(nsaddr_t nexthopid, nsaddr_t id)
{
	antconf_rt_entry *rt = rthead.lh_first;
	for(; rt; rt = rt->rt_link.le_next) 
	{
		if((rt->rt_dst == id)&&(rt->rt_nexthop==nexthopid))
		break;
	}
	 return rt;
}


antconf_rt_entry*
antconf_rtable::rt_lookup(nsaddr_t id)
{
	antconf_rt_entry *rt = rthead.lh_first;
	for(; rt; rt = rt->rt_link.le_next) 
	{
		if(rt->rt_dst == id)
		break;
	}
	 return rt;
}

antconf_rt_entry*
antconf_rtable::rt_lookupoptimal(nsaddr_t id)
{
	antconf_rt_entry *rt = rthead.lh_first;
        antconf_rt_entry *rt0 ;

	int i=0;
        u_int16_t d;
	u_int32_t seqno=0;
	bool exist=false;

	for(; rt; rt = rt->rt_link.le_next) 
	{
            if(i==0)
	    {
		if((rt->rt_dst == id)&&(rt->rt_flags == RTF_UP))
		{
		  d=rt->rt_hops;
		  seqno=rt->rt_seqno;
		  rt0=rt;
		  exist=true;
		  i++;
		}
	    }
	    else if((rt->rt_dst == id)&&(rt->rt_flags == RTF_UP)&&((rt->rt_seqno>seqno)||((rt->rt_seqno==seqno)&&(rt->rt_hops<d))))
	   {
		  d=rt->rt_hops;
		  seqno=rt->rt_seqno;
		  rt0=rt;
		  exist=true;		
	   }			
	}
	if(exist) return rt0;
   else return 0;
}

void
antconf_rtable::rt_delete(nsaddr_t nexthopid, nsaddr_t id)
{
	antconf_rt_entry *rt = rt_lookup(nexthopid,id);
	if(rt) 
	{
		LIST_REMOVE(rt, rt_link);
		delete rt;
	}

}

antconf_rt_entry*
antconf_rtable::rt_add(nsaddr_t nexthopid, nsaddr_t id)
{
	antconf_rt_entry *rt;
	assert(rt_lookup(id) == 0);
	rt = new antconf_rt_entry;
	assert(rt);
	rt->rt_dst = id;
        rt->rt_nexthop = nexthopid;
	LIST_INSERT_HEAD(&rthead, rt, rt_link);
	return rt;
}

antconf_rt_entry*
antconf_rtable::rt_add(nsaddr_t id)
{
antconf_rt_entry *rt;

 assert(rt_lookup(id) == 0);
 rt = new antconf_rt_entry;
 assert(rt);
 rt->rt_dst = id;
 LIST_INSERT_HEAD(&rthead, rt, rt_link);
 return rt;
}
