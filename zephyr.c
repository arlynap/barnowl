#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

#ifdef HAVE_LIBZEPHYR
Code_t ZResetAuthentication();
#endif

int owl_zephyr_initialize()
{
#ifdef HAVE_LIBZEPHYR
  int ret;
  
  if ((ret = ZInitialize()) != ZERR_NONE) {
    com_err("owl",ret,"while initializing");
    return(1);
  }
  if ((ret = ZOpenPort(NULL)) != ZERR_NONE) {
    com_err("owl",ret,"while opening port");
    return(1);
  }
#endif
  return(0);
}


int owl_zephyr_shutdown()
{
#ifdef HAVE_LIBZEPHYR
  unsuball();
  ZClosePort();
#endif
  return(0);
}

int owl_zephyr_zpending()
{
#ifdef HAVE_LIBZEPHYR
  return(ZPending());
#else
  return(0);
#endif
}

char *owl_zephyr_get_realm()
{
#ifdef HAVE_LIBZEPHYR
  return(ZGetRealm());
#else
  return("");
#endif
}

char *owl_zephyr_get_sender()
{
#ifdef HAVE_LIBZEPHYR
  return(ZGetSender());
#else
  return("");
#endif
}

int owl_zephyr_loadsubs(char *filename)
{
#ifdef HAVE_LIBZEPHYR
  /* return 0  on success
   *        -1 on file error
   *        -2 on subscription error
   */
  FILE *file;
  char *tmp, *start;
  char buffer[1024], subsfile[1024];
  ZSubscription_t subs[3001];
  int count, ret, i;

  ret=0;
  
  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".zephyr.subs");
  } else {
    strcpy(subsfile, filename);
  }

  ZResetAuthentication();
  /* need to redo this to do chunks, not just bail after 3000 */
  count=0;
  file=fopen(subsfile, "r");
  if (file) {
    while ( fgets(buffer, 1024, file)!=NULL ) {
      if (buffer[0]=='#' || buffer[0]=='\n' || buffer[0]=='\n') continue;

      if (buffer[0]=='-') {
	start=buffer+1;
      } else {
	start=buffer;
      }
      
      if (count >= 3000) break; /* also tell the user */

      /* add it to the list of subs */
      if ((tmp=(char *) strtok(start, ",\n\r"))==NULL) continue;
      subs[count].zsub_class=owl_strdup(tmp);
      if ((tmp=(char *) strtok(NULL, ",\n\r"))==NULL) continue;
      subs[count].zsub_classinst=owl_strdup(tmp);
      if ((tmp=(char *) strtok(NULL, " \t\n\r"))==NULL) continue;
      subs[count].zsub_recipient=owl_strdup(tmp);

      /* if it started with '-' then add it to the global punt list */
      if (buffer[0]=='-') {
	owl_function_zpunt(subs[count].zsub_class, subs[count].zsub_classinst, subs[count].zsub_recipient, 0);
      }
      
      count++;
    }
    fclose(file);
  } else {
    count=0;
    ret=-1;
  }

  /* sub with defaults */
  if (ZSubscribeTo(subs,count,0) != ZERR_NONE) {
    fprintf(stderr, "Error subbing\n");
    ret=-2;
  }

  /* free stuff */
  for (i=0; i<count; i++) {
    owl_free(subs[i].zsub_class);
    owl_free(subs[i].zsub_classinst);
    owl_free(subs[i].zsub_recipient);
  }

  return(ret);
#else
  return(0);
#endif
}

int owl_zephyr_loadloginsubs(char *filename)
{
#ifdef HAVE_LIBZEPHYR
  FILE *file;
  ZSubscription_t subs[3001];
  char subsfile[1024], buffer[1024];
  int count, ret, i;

  ret=0;

  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".anyone");
  } else {
    strcpy(subsfile, filename);
  }

  ZResetAuthentication();
  /* need to redo this to do chunks, not just bag out after 3000 */
  count=0;
  file=fopen(subsfile, "r");
  if (file) {
    while ( fgets(buffer, 1024, file)!=NULL ) {
      if (buffer[0]=='#' || buffer[0]=='\n' || buffer[0]=='\n') continue;
      
      if (count >= 3000) break; /* also tell the user */

      buffer[strlen(buffer)-1]='\0';
      subs[count].zsub_class="login";
      subs[count].zsub_recipient="*";
      if (strchr(buffer, '@')) {
	subs[count].zsub_classinst=owl_strdup(buffer);
      } else {
	subs[count].zsub_classinst=owl_sprintf("%s@%s", buffer, ZGetRealm());
      }

      count++;
    }
    fclose(file);
  } else {
    count=0;
    ret=-1;
  }

  /* sub with defaults */
  if (ZSubscribeToSansDefaults(subs,count,0) != ZERR_NONE) {
    fprintf(stderr, "Error subbing\n");
    ret=-2;
  }

  /* free stuff */
  for (i=0; i<count; i++) {
    owl_free(subs[i].zsub_classinst);
  }

  return(ret);
#else
  return(0);
#endif
}

void unsuball()
{
#if HAVE_LIBZEPHYR
  int ret;

  ZResetAuthentication();
  ret=ZCancelSubscriptions(0);
  if (ret != ZERR_NONE) {
    com_err("owl",ret,"while unsubscribing");
  }
#endif
}

int owl_zephyr_sub(char *class, char *inst, char *recip)
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t subs[5];
  int ret;

  subs[0].zsub_class=class;
  subs[0].zsub_classinst=inst;
  subs[0].zsub_recipient=recip;

  ZResetAuthentication();
  if (ZSubscribeTo(subs,1,0) != ZERR_NONE) {
    fprintf(stderr, "Error subbing\n");
    ret=-2;
  }
  return(0);
#else
  return(0);
#endif
}


int owl_zephyr_unsub(char *class, char *inst, char *recip)
{
#ifdef HAVE_LIBZEPHYR
  ZSubscription_t subs[5];
  int ret;

  subs[0].zsub_class=class;
  subs[0].zsub_classinst=inst;
  subs[0].zsub_recipient=recip;

  ZResetAuthentication();
  if (ZUnsubscribeTo(subs,1,0) != ZERR_NONE) {
    fprintf(stderr, "Error unsubbing\n");
    ret=-2;
  }
  return(0);
#else
  return(0);
#endif
}

#ifdef HAVE_LIBZEPHYR
char *owl_zephyr_get_field(ZNotice_t *n, int j, int *k)
{
  /* return a pointer to the Jth field, place the length in k.  If the
     field doesn't exist return an emtpy string */
  int i, count, save;

  count=save=0;
  for (i=0; i<n->z_message_len; i++) {
    if (n->z_message[i]=='\0') {
      count++;
      if (count==j) {
	/* just found the end of the field we're looking for */
	*k=i-save;
	return(n->z_message+save);
      } else {
	save=i+1;
      }
    }
  }
  /* catch the last field */
  if (count==j-1) {
    *k=n->z_message_len-save;
    return(n->z_message+save);
  }
  
  *k=0;
  return("");
}
#else
char *owl_zephyr_get_field(void *n, int j, int *k)
{
  return("");
}
#endif

#ifdef HAVE_LIBZEPHYR
int owl_zephyr_get_num_fields(ZNotice_t *n)
{
  int i, fields;

  fields=1;
  for (i=0; i<n->z_message_len; i++) {
    if (n->z_message[i]=='\0') fields++;
  }
  
  return(fields);
}
#else
int owl_zephyr_get_num_fields(void *n)
{
  return(0);
}
#endif

#ifdef HAVE_LIBZEPHYR
char *owl_zephyr_get_message(ZNotice_t *n, int *k)
{
  /* return a pointer to the message, place the message length in k */
  if (!strcasecmp(n->z_opcode, "ping")) {
    *k=0;
    return("");
  }

  return(owl_zephyr_get_field(n, 2, k));
}
#endif

#ifdef HAVE_LIBZEPHYR
char *owl_zephyr_get_zsig(ZNotice_t *n, int *k)
{
  /* return a pointer to the zsig if there is one */

  if (n->z_message_len==0) {
    *k=0;
    return("");
  }
  *k=strlen(n->z_message);
  return(n->z_message);
}
#else
char *owl_zephyr_get_zsig(void *n, int *k)
{
  return("");
}
#endif

int send_zephyr(char *opcode, char *zsig, char *class, char *instance, char *recipient, char *message)
{
#ifdef HAVE_LIBZEPHYR
  int ret;
  ZNotice_t notice;
    
  memset(&notice, 0, sizeof(notice));

  ZResetAuthentication();
  
  notice.z_kind=ACKED;
  notice.z_port=0;
  notice.z_class=class;
  notice.z_class_inst=instance;
  if (!strcmp(recipient, "*") || !strcmp(recipient, "@")) {
    notice.z_recipient="";
  } else {
    notice.z_recipient=recipient;
  }
  notice.z_default_format="Class $class, Instance $instance:\nTo: @bold($recipient) at $time $date\nFrom: @bold{$1 <$sender>}\n\n$2";
  notice.z_sender=NULL;
  if (opcode) notice.z_opcode=opcode;

  notice.z_message_len=strlen(zsig)+1+strlen(message);
  notice.z_message=owl_malloc(notice.z_message_len+10);
  strcpy(notice.z_message, zsig);
  memcpy(notice.z_message+strlen(zsig)+1, message, strlen(message));

  /* ret=ZSendNotice(&notice, ZAUTH); */
  ret=ZSrvSendNotice(&notice, ZAUTH, send_zephyr_helper);
  
  /* free then check the return */
  owl_free(notice.z_message);
  ZFreeNotice(&notice);
  if (ret!=ZERR_NONE) {
    owl_function_error("Error sending zephyr");
    return(ret);
  }
  return(0);
#else
  return(0);
#endif
}

#ifdef HAVE_LIBZEPHYR
Code_t send_zephyr_helper(ZNotice_t *notice, char *buf, int len, int wait)
{
  return(ZSendPacket(buf, len, 0));
}
#endif

void send_ping(char *to)
{
#ifdef HAVE_LIBZEPHYR
  send_zephyr("PING", "", "MESSAGE", "PERSONAL", to, "");
#endif
}

#ifdef HAVE_LIBZEPHYR
void owl_zephyr_handle_ack(ZNotice_t *retnotice)
{
  char *tmp;
  
  /* if it's an HMACK ignore it */
  if (retnotice->z_kind == HMACK) return;

  if (retnotice->z_kind == SERVNAK) {
    owl_function_error("Authorization failure sending zephyr");
  } else if ((retnotice->z_kind != SERVACK) || !retnotice->z_message_len) {
    owl_function_error("Detected server failure while receiving acknowledgement");
  } else if (!strcmp(retnotice->z_message, ZSRVACK_SENT)) {
    if (!strcasecmp(retnotice->z_opcode, "ping")) {
      return;
    } else if (!strcasecmp(retnotice->z_class, "message") &&
	       !strcasecmp(retnotice->z_class_inst, "personal")) {
      tmp=short_zuser(retnotice->z_recipient);
      owl_function_makemsg("Message sent to %s.", tmp);
      free(tmp);
    } else {
      owl_function_makemsg("Message sent to -c %s -i %s\n", retnotice->z_class, retnotice->z_class_inst);
    }
  } else if (!strcmp(retnotice->z_message, ZSRVACK_NOTSENT)) {
    if (strcasecmp(retnotice->z_class, "message")) {
      char buff[1024];
      owl_function_error("No one subscribed to class class %s", retnotice->z_class);
      sprintf(buff, "Could not send message to class %s: no one subscribed.\n", retnotice->z_class);
      owl_function_adminmsg("", buff);
    } else {
      char buff[1024];
      tmp = short_zuser(retnotice->z_recipient);
      owl_function_error("%s: Not logged in or subscribing to messages.", 
			   tmp);

      sprintf(buff, "Could not send message to %s: not logged in or subscribing to messages.\n", tmp);
      owl_function_adminmsg("", buff);
      owl_free(tmp);
    }
  } else {
    owl_function_error("Internal error on ack (%s)", retnotice->z_message);
  }
}
#else
void owl_zephyr_handle_ack(void *retnotice)
{
}
#endif

#ifdef HAVE_LIBZEPHYR
int owl_zephyr_notice_is_ack(ZNotice_t *n)
{
  if (n->z_kind == SERVNAK || n->z_kind == SERVACK || n->z_kind == HMACK) {
    if (!strcasecmp(n->z_class, LOGIN_CLASS)) return(0);
    return(1);
  }
  return(0);
}
#else
int owl_zephyr_notice_is_ack(void *n)
{
  return(0);
}
#endif
  
void owl_zephyr_zaway(owl_message *m)
{
#ifdef HAVE_LIBZEPHYR
  char *tmpbuff, *myuser, *to;
  
  /* bail if it doesn't look like a message we should reply to.  Some
     of this defined by the way zaway(1) works */
  if (strcasecmp(owl_message_get_class(m), "message")) return;
  if (strcasecmp(owl_message_get_recipient(m), ZGetSender())) return;
  if (!strcasecmp(owl_message_get_sender(m), "")) return;
  if (!strcasecmp(owl_message_get_opcode(m), "ping")) return;
  if (!strcasecmp(owl_message_get_opcode(m), "auto")) return;
  if (!strcasecmp(owl_message_get_zsig(m), "Automated reply:")) return;
  if (!strcasecmp(owl_message_get_sender(m), ZGetSender())) return;

  if (owl_global_is_smartstrip(&g)) {
    to=owl_zephyr_smartstripped_user(owl_message_get_sender(m));
  } else {
    to=owl_strdup(owl_message_get_sender(m));
  }

  send_zephyr("",
	      "Automated reply:",
	      owl_message_get_class(m),
	      owl_message_get_instance(m),
	      to,
	      owl_global_get_zaway_msg(&g));

  myuser=short_zuser(to);
  if (!strcasecmp(owl_message_get_instance(m), "personal")) {
    tmpbuff = owl_sprintf("zwrite %s", myuser);
  } else {
    tmpbuff = owl_sprintf("zwrite -i %s %s", owl_message_get_instance(m), myuser);
  }
  owl_free(myuser);
  owl_free(to);

  /* display the message as an admin message in the receive window */
  owl_function_make_outgoing_zephyr(owl_global_get_zaway_msg(&g), tmpbuff, "Automated reply:");
  owl_free(tmpbuff);
#endif
}

#ifdef HAVE_LIBZEPHYR
void owl_zephyr_hackaway_cr(ZNotice_t *n)
{
  /* replace \r's with ' '.  Gross-ish */
  int i;

  for (i=0; i<n->z_message_len; i++) {
    if (n->z_message[i]=='\r') {
      n->z_message[i]=' ';
    }
  }
}
#endif

void owl_zephyr_zlocate(char *user, char *out, int auth)
{
#ifdef HAVE_LIBZEPHYR
  int ret, numlocs;
  int one = 1;
  ZLocations_t locations;
  char *myuser;
  
  strcpy(out, "");
  ZResetAuthentication();
  ret=ZLocateUser(user,&numlocs,auth?ZAUTH:ZNOAUTH);
  if (ret != ZERR_NONE) {
    owl_function_error("Error locating user %s", user);
  }

  if (numlocs==0) {
    myuser=short_zuser(user);
    sprintf(out, "%s: Hidden or not logged-in\n", myuser);
    owl_free(myuser);
    return;
  }
    
  for (;numlocs;numlocs--) {
    ZGetLocations(&locations,&one);
    myuser=short_zuser(user);
    sprintf(out, "%s%s: %s\t%s\t%s\n", out, myuser, locations.host, locations.tty, locations.time);
    owl_free(myuser);
  }
#endif
}

void owl_zephyr_addsub(char *filename, char *class, char *inst, char *recip)
{
#ifdef HAVE_LIBZEPHYR
  char *line, subsfile[LINE], buff[LINE];
  FILE *file;

  line=owl_zephyr_makesubline(class, inst, recip);

  if (filename==NULL) {
    sprintf(subsfile, "%s/%s", owl_global_get_homedir(&g), ".zephyr.subs");
  } else {
    strcpy(subsfile, filename);
  }

  /* first check if it exists already */
  file=fopen(subsfile, "r");
  if (!file) {
    owl_function_error("Error opening file %s", subsfile);
    owl_free(line);
    return;
  }
  while (fgets(buff, LINE, file)!=NULL) {
    if (!strcasecmp(buff, line)) {
      owl_function_error("Subscription already present in %s", subsfile);
      owl_free(line);
      return;
    }
  }

  /* if we get here then we didn't find it */
  fclose(file);
  file=fopen(subsfile, "a");
  if (!file) {
    owl_function_error("Error opening file %s for writing", subsfile);
    owl_free(line);
    return;
  }
  fputs(line, file);
  fclose(file);
  owl_function_makemsg("Subscription added");
  
  owl_free(line);
#endif
}

void owl_zephyr_delsub(char *filename, char *class, char *inst, char *recip)
{
#ifdef HAVE_LIBZEPHYR
  char *line, *subsfile;
  
  line=owl_zephyr_makesubline(class, inst, recip);
  line[strlen(line)-1]='\0';

  if (!filename) {
    subsfile=owl_sprintf("%s/.zephyr.subs", owl_global_get_homedir(&g));
  } else {
    subsfile=owl_strdup(filename);
  }
  
  owl_util_file_deleteline(subsfile, line, 1);
  owl_free(subsfile);
  owl_free(line);
  owl_function_makemsg("Subscription removed");
#endif
}

/* caller must free the return */
char *owl_zephyr_makesubline(char *class, char *inst, char *recip)
{
  char *out;

  out=owl_malloc(strlen(class)+strlen(inst)+strlen(recip)+30);
  sprintf(out, "%s,%s,%s\n", class, inst, !strcmp(recip, "") ? "*" : recip);
  return(out);
}


void owl_zephyr_zlog_in(void)
{
#ifdef HAVE_LIBZEPHYR
  char *exposure, *eset;
  int ret;

  ZResetAuthentication();
    
  eset=EXPOSE_REALMVIS;
  exposure=ZGetVariable("exposure");
  if (exposure==NULL) {
    eset=EXPOSE_REALMVIS;
  } else if (!strcasecmp(exposure,EXPOSE_NONE)) {
    eset = EXPOSE_NONE;
  } else if (!strcasecmp(exposure,EXPOSE_OPSTAFF)) {
    eset = EXPOSE_OPSTAFF;
  } else if (!strcasecmp(exposure,EXPOSE_REALMVIS)) {
    eset = EXPOSE_REALMVIS;
  } else if (!strcasecmp(exposure,EXPOSE_REALMANN)) {
    eset = EXPOSE_REALMANN;
  } else if (!strcasecmp(exposure,EXPOSE_NETVIS)) {
    eset = EXPOSE_NETVIS;
  } else if (!strcasecmp(exposure,EXPOSE_NETANN)) {
    eset = EXPOSE_NETANN;
  }
   
  ret=ZSetLocation(eset);
  if (ret != ZERR_NONE) {
    /*
      char buff[LINE];
      sprintf(buff, "Error setting location: %s", error_message(ret));
      owl_function_makemsg(buff);
    */
  }
#endif
}

void owl_zephyr_zlog_out(void)
{
#ifdef HAVE_LIBZEPHYR
  int ret;

  ZResetAuthentication();
  ret=ZUnsetLocation();
  if (ret != ZERR_NONE) {
    /*
      char buff[LINE];
      sprintf(buff, "Error unsetting location: %s", error_message(ret));
      owl_function_makemsg(buff);
    */
  }
#endif
}

void owl_zephyr_addbuddy(char *name)
{
  char *filename;
  FILE *file;
  
  filename=owl_sprintf("%s/.anyone", owl_global_get_homedir(&g));
  file=fopen(filename, "a");
  owl_free(filename);
  if (!file) {
    owl_function_error("Error opening zephyr buddy file for append");
    return;
  }
  fprintf(file, "%s\n", name);
  fclose(file);
}

void owl_zephyr_delbuddy(char *name)
{
  char *filename;

  filename=owl_sprintf("%s/.anyone", owl_global_get_homedir(&g));
  owl_util_file_deleteline(filename, name, 0);
  owl_free(filename);
}

/* return auth string */
#ifdef HAVE_LIBZEPHYR
char *owl_zephyr_get_authstr(ZNotice_t *n)
{

  if (!n) return("UNKNOWN");

  if (n->z_auth == ZAUTH_FAILED) {
    return ("FAILED");
  } else if (n->z_auth == ZAUTH_NO) {
    return ("NO");
  } else if (n->z_auth == ZAUTH_YES) {
    return ("YES");
  } else {
    return ("UNKNOWN");
  }           
}
#else
char *owl_zephyr_get_authstr(void *n)
{
  return("");
}
#endif

/* returns a buffer of subscriptions or an error message.
 * Caller must free the return.
 */
char *owl_zephyr_getsubs()
{
#ifdef HAVE_LIBZEPHYR
  int ret, num, i, one;
  ZSubscription_t sub;
  char *out, *tmpbuff;
  one=1;

  ret=ZRetrieveSubscriptions(0, &num);
  if (ret==ZERR_TOOMANYSUBS) {
    out=owl_strdup("Zephyr: too many subscriptions\n");
    return(out);
  }

  out=owl_malloc(num*500);
  tmpbuff=owl_malloc(num*500);
  strcpy(out, "");
  for (i=0; i<num; i++) {
    if ((ret = ZGetSubscriptions(&sub, &one)) != ZERR_NONE) {
      owl_free(out);
      owl_free(tmpbuff);
      ZFlushSubscriptions();
      out=owl_strdup("Error while getting subscriptions\n");
      return(out);
    } else {
      sprintf(tmpbuff, "<%s,%s,%s>\n%s", sub.zsub_class, sub.zsub_classinst, sub.zsub_recipient, out);
      strcpy(out, tmpbuff);
    }
  }

  owl_free(tmpbuff);
  ZFlushSubscriptions();
  return(out);
#else
  return("");
#endif
}

char *owl_zephyr_get_variable(char *var)
{
#ifdef HAVE_LIBZEPHYR
  return(ZGetVariable(var));
#else
  return("");
#endif
}

void owl_zephyr_set_locationinfo(char *host, char *val)
{
#ifdef HAVE_LIBZEPHYR
  ZInitLocationInfo(host, val);
#endif
}
  


/* Strip a local realm fron the zephyr user name.
 * The caller must free the return
 */
char *short_zuser(char *in)
{
  char *out, *ptr;

  out=owl_strdup(in);
  ptr=strchr(out, '@');
  if (ptr) {
    if (!strcasecmp(ptr+1, owl_zephyr_get_realm())) {
      *ptr='\0';
    }
  }
  return(out);
}

/* Append a local realm to the zephyr user name if necessary.
 * The caller must free the return.
 */
char *long_zuser(char *in)
{
  char *ptr;

  if (NULL != (ptr=strchr(in, '@'))) {
    return owl_strdup(in);
  } else {
    return owl_sprintf("%s@%s", in, owl_zephyr_get_realm());
  }
}


/* strip out the instance from a zsender's principal.  Preserves the
 * realm if present.  daemon.webzephyr is a special case.  The
 * caller must free the return
 */
char *owl_zephyr_smartstripped_user(char *in)
{
  char *ptr, *realm, *out;

  out=owl_strdup(in);

  /* bail immeaditly if we don't have to do any work */
  ptr=strchr(in, '.');
  if (!strchr(in, '/') && !ptr) {
    /* no '/' and no '.' */
    return(out);
  }
  if (ptr && strchr(in, '@') && (ptr > strchr(in, '@'))) {
    /* There's a '.' but it's in the realm */
    return(out);
  }
  if (!strncasecmp(in, OWL_WEBZEPHYR_PRINCIPAL, strlen(OWL_WEBZEPHYR_PRINCIPAL))) {
    return(out);
  }

  /* remove the realm from ptr, but hold on to it */
  realm=strchr(out, '@');
  if (realm) realm[0]='\0';

  /* strip */
  ptr=strchr(out, '.');
  if (!ptr) ptr=strchr(out, '/');
  ptr[0]='\0';

  /* reattach the realm if we had one */
  if (realm) {
    strcat(out, "@");
    strcat(out, realm+1);
  }

  return(out);
}
