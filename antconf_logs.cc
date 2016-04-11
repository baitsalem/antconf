// authors:
// AIT SALEM Boussad
//  Riahla Amine

#include <antconf/antconf.h>
#include <antconf/antconf_packet.h>
#include <ip.h>
#define CURRENT_TIME    Scheduler::instance().clock()

static const int verbose = 0;

/* =====================================================================
   Functions de journalisation
   ===================================================================== */
void
ANTCONF::log_link_del(nsaddr_t dst)
{
        static int link_del = 0;

        if(! logtarget || ! verbose) return;

        sprintf(logtarget->pt_->buffer(),
                "A %.9f _%d_ deleting LL hop to %d (delete %d is %s)",
                CURRENT_TIME,
                index,
                dst,
                ++link_del,
                God::instance()->hops(index, dst) != 1 ? "VALID" : "INVALID");
        logtarget->pt_->dump();
}


void
ANTCONF::log_link_broke(Packet *p)
{
        static int link_broke = 0;
        struct hdr_cmn *ch = HDR_CMN(p);

        if(! logtarget || ! verbose) return;

        sprintf(logtarget->pt_->buffer(),
                "A %.9f _%d_ LL unable to deliver packet %d to %d (%d) (reason = %d, ifqlen = %d)",
                CURRENT_TIME,
                index,
                ch->uid_,
                ch->next_hop_,
                ++link_broke,
                ch->xmit_reason_,
                ifqueue->length());
	logtarget->pt_->dump();
}

void
ANTCONF::log_link_kept(nsaddr_t dst)
{
        static int link_kept = 0;
        if(! logtarget || ! verbose) return;

        sprintf(logtarget->pt_->buffer(),
                "A %.9f _%d_ keeping LL hop to %d (keep %d is %s)",
                CURRENT_TIME,
                index,
                dst,
                ++link_kept,
                God::instance()->hops(index, dst) == 1 ? "VALID" : "INVALID");
        logtarget->pt_->dump();
}

