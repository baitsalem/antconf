#include <assert.h>

#include <cmu-trace.h>
#include <antconf/antconf_rqueue.h>

#define CURRENT_TIME    Scheduler::instance().clock()
#define QDEBUG

/*
  File d'attente utilisée par ANTCONF.
*/

antconf_rqueue::antconf_rqueue() 
{
	head_ = tail_ = 0;
	len_ = 0;
	limit_ = ANTCONF_RTQ_MAX_LEN;
	timeout_ = ANTCONF_RTQ_TIMEOUT;
}

void
antconf_rqueue::enque(Packet *p) 
{
	struct hdr_cmn *ch = HDR_CMN(p);
	/*Purger la file.*/
	purge(); 
	p->next_ = 0;
	ch->ts_ = CURRENT_TIME + timeout_;
	if (len_ == limit_) 
	{
		Packet *p0 = remove_head();	
	   assert(p0);
	   if(HDR_CMN(p0)->ts_ > CURRENT_TIME) 
	  {
		 drop(p0, DROP_RTR_QFULL);
	   }
       else 
	  {
		drop(p0, DROP_RTR_QTIMEOUT);
	  }
    } 
    if(head_ == 0) 
    {
      head_ = tail_ = p;
    }
    else 
   {
     tail_->next_ = p;
     tail_ = p;
   }
    len_++;
    #ifdef QDEBUG
		verifyQueue();
    #endif // QDEBUG
}
                

Packet*
antconf_rqueue::deque() 
{
	//fprintf(stderr, "droping at %2f...\n", Scheduler::instance().clock());
	Packet *p;
	/*Purger la file.*/
	purge();
	p = remove_head();
	#ifdef QDEBUG
		verifyQueue();
	#endif // QDEBUG
	return p;
}


Packet*
antconf_rqueue::deque(nsaddr_t dst) 
{
	
	//fprintf(stderr, "droping dest %d at %2f...\n", dst,Scheduler::instance().clock());
	Packet *p, *prev;
	/*Purger la file.*/
	purge();
	findPacketWithDst(dst, p, prev);
	assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));
	if(p == 0) return 0;
	if (p == head_) 
	{
		p = remove_head();
	}
	else if (p == tail_) 
	{
		prev->next_ = 0;
		tail_ = prev;
		len_--;
	}
	else 
	{
		prev->next_ = p->next_;
		len_--;
	}
	#ifdef QDEBUG
		verifyQueue();
   #endif // QDEBUG
	return p;
}

char 
antconf_rqueue::find(nsaddr_t dst) 
{
  Packet *p, *prev;  
  findPacketWithDst(dst, p, prev);
  if (0 == p)
    return 0;
  else
   return 1;
}

Packet*
antconf_rqueue::remove_head() 
{
	Packet *p = head_;
    if(head_ == tail_) 
	{
		head_ = tail_ = 0;
	}
	else 
	{
		head_ = head_->next_;
	}
	if(p) len_--;
	return p;
}

void
antconf_rqueue::findPacketWithDst(nsaddr_t dst, Packet*& p, Packet*& prev) 
{
  p = prev = 0;
  for(p = head_; p; p = p->next_) 
 {
	 
	  if(HDR_IP(p)->daddr() == dst) 
	  {
            return;
          }
          prev = p;
  }
}


void
antconf_rqueue::verifyQueue() 
{
	Packet *p, *prev = 0;
	int cnt = 0;
	for(p = head_; p; p = p->next_) 
	{
		cnt++;
		prev = p;
	}
	assert(cnt == len_);
	assert(prev == tail_);
}

bool
antconf_rqueue::findAgedPacket(Packet*& p, Packet*& prev) 
{
  p = prev = 0;
  for(p = head_; p; p = p->next_) 
 {
    if(HDR_CMN(p)->ts_ < CURRENT_TIME) 
    {
      return true;
    }
    prev = p;
 }
  return false;
}

void
antconf_rqueue::purge() 
{
	Packet *p, *prev;
	while ( findAgedPacket(p, prev) ) 
	{
 		assert(p == 0 || (p == head_ && prev == 0) || (prev->next_ == p));
 		if(p == 0) return;
 		if (p == head_) 
		{
   			p = remove_head();
 		}
 		else if (p == tail_) 
		{
   			prev->next_ = 0;
   			tail_ = prev;
   			len_--;
 		}
 		else 
		{
   			prev->next_ = p->next_;
   			len_--;
 		}
		#ifdef QDEBUG
 			verifyQueue();
		#endif // QDEBUG
		p = prev = 0;
	}

}

