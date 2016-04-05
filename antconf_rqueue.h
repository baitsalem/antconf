#ifndef __antconf_rqueue_h__
#define __antconf_rqueue_h_


#include <ip.h>
#include <agent.h>

/*
 * taille maximal de la file d'attente
 */
#define ANTCONF_RTQ_MAX_LEN   100 //  64      // packets

/*
 *  durée de vie d'un paquet dans la file d'attente
 */
#define ANTCONF_RTQ_TIMEOUT     3	// seconds

class antconf_rqueue : public Connector 
{
 public:
        antconf_rqueue();

        void            recv(Packet *, Handler*) { abort(); }

        void            enque(Packet *p);

	inline int      command(int argc, const char * const* argv) 
	    { return Connector::command(argc, argv); }

        /*
         *  retourne le paquet a l'entète
         */
        Packet*         deque(void);

        /*
         * retourné le paquet pour la destination "D".
         */
        Packet*         deque(nsaddr_t dst);
        /*
       * voir si le paquet pour la destination "dst" existe 
       */
        char            find(nsaddr_t dst);

 private:
	//les autres fonctions de  gestion de la file
        Packet*         remove_head();
        void            purge(void);
	void		findPacketWithDst(nsaddr_t dst, Packet*& p, Packet*& prev);
	bool 		findAgedPacket(Packet*& p, Packet*& prev); 
	void		verifyQueue(void);

        Packet          *head_;
        Packet          *tail_;

        int             len_;

        int             limit_;
        double          timeout_;

};

#endif /* __antconf_rqueue_h__ */
