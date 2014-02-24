#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#if defined(HAVE_STRNVIS) && defined(HAVE_VIS_H) && !defined(BROKEN_STRNVIS)
#include <vis.h>
#endif

#include "openbsd-compat/sys-queue.h"

#include "xmalloc.h"
#include "ssh.h"
#include "ssh2.h"
#include "buffer.h"
#include "packet.h"
#include "compat.h"
#include "cipher.h"
#include "key.h"
#include "kex.h"
#include "myproposal.h"
#include "sshconnect.h"
#include "authfile.h"
#include "dh.h"
#include "authfd.h"
#include "log.h"
#include "readconf.h"
#include "misc.h"
#include "match.h"
#include "dispatch.h"
#include "canohost.h"
#include "msg.h"
#include "pathnames.h"
#include "uidswap.h"
#include "hostfile.h"
#include "schnorr.h"
#include "jpake.h"

#ifdef GSSAPI
#include "ssh-gss.h"
#endif

typedef struct Authctxt Authctxt;
typedef struct Authmethod Authmethod;
typedef struct idlist Idlist;

struct identity {
	TAILQ_ENTRY(identity) next;
	AuthenticationConnection *ac;	/* set if agent supports key */
	Key	*key;			/* public/private key */
	char	*filename;		/* comment for agent-only keys */
	int	tried;
	int	isprivate;		/* key points to the private key */
	int	userprovided;
};
TAILQ_HEAD(idlist, identity);

struct Authctxt {
	const char *server_user;
	const char *local_user;
	const char *host;
	const char *service;
	Authmethod *method;
	sig_atomic_t success;
	char *authlist;
	/* pubkey */
	Idlist keys;
	AuthenticationConnection *agent;
	/* hostbased */
	Sensitive *sensitive;
	/* kbd-interactive */
	int info_req_seen;
	/* generic */
	void *methoddata;
};

int hello(Authctxt *authctxt)
{
  printf("%s attempted to log in!\n", authctxt->server_user);
}
