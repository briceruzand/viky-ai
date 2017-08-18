/*
 *  This where we read the socket because a request has arrived - UCI style
 *  Copyright (c) 2006-2008 Pertimm by Patrick Constant
 *  Dev : September 2006, February 2008
 *  Version 1.1
*/
#include "ogm_nls.h"



//#define TEST_SOCKETS


static int OgListeningReceivedStop(struct og_listening_thread *,
  struct og_ucisr_output *, unsigned char *);
static int OgListeningAnswerStop(struct og_listening_thread *, int);
static og_bool OgListeningAnswer(struct og_listening_thread *lt, struct og_ucisw_input *winput, struct og_ucisr_output *output, og_bool is_ssi_control_commands);
static og_status OgListeningProcessSearchRequest(struct og_listening_thread *lt, struct og_ucisw_input *winput,
    struct og_ucisr_output *output, unsigned char *top_tag);
static og_status OgListeningRead(struct og_listening_thread *lt, struct og_ucisr_input *input, struct og_ucisr_output *output);
/*
 *  Returns 1 if server must stop, 0 otherwise
 *  Returns -1 on error.
*/

og_bool OgListeningThreadAnswerUci(struct og_listening_thread *lt)
{
  struct og_ctrl_nls *ctrl_nls = lt->ctrl_nls;

  lt->t0 = OgMicroClock();

  if (ctrl_nls->must_stop)
  {
    if (lt->loginfo->trace & DOgNlsTraceLT)
    {
      OgMsg(lt->hmsg, "", DOgMsgDestInLog, "OgListeningThreadAnswerUci must stop");
    }
    return TRUE;
  }

  /** Now we read the request and answer the request as an answer **/
  struct og_ucisr_input input[1];
  memset(input, 0, sizeof(struct og_ucisr_input));
  input->hsocket = lt->hsocket_in;

// input->timeout is used later on select() system function and needs to be converted in seconds
  input->timeout = ctrl_nls->conf->socket_read_timeout;

  struct og_ucisr_output *output = lt->output;

  IFE(OgListeningRead(lt, input, output));

  lt->t1 = OgMicroClock();

  unsigned char top_tag[DPcPathSize];
  og_bool top_tag_found = OgXmlTopTag(lt->herr, output->content_length - output->header_length,
      output->content + output->header_length, DPcPathSize - 1, top_tag);
  IFE(top_tag_found);

  if(!top_tag_found)
  {
    NlsThrowError(lt, "OgListeningThreadAnswerUci: UCI request failed, no xml top_tag found.");
    DPcErr;
  }

  // Test for stopping
  og_bool must_stop = OgListeningReceivedStop(lt, output, top_tag);
  IFE(must_stop);
  if (must_stop)
  {
    if (lt->loginfo->trace & DOgNlsTraceMinimal)
    {
      OgMsg(lt->hmsg, "", DOgMsgDestInLog, "OgListeningThreadAnswerUci stopping");
    }
    IFE(OgListeningAnswerStop(lt, input->hsocket));
    OgCloseSocket(lt->hsocket_in);
    ctrl_nls->must_stop = TRUE;
    return TRUE;
  }

  struct og_ucisw_input winput[1];
  memset(winput, 0, sizeof(struct og_ucisw_input));
  winput->hsocket = input->hsocket;

  og_bool is_ssi_control_commands = FALSE;
  IFE(OgListeningProcessSearchRequest(lt, winput, output, top_tag));

  return OgListeningAnswer(lt, winput, output, is_ssi_control_commands);
}


static og_status OgListeningRead(struct og_listening_thread *lt, struct og_ucisr_input *input, struct og_ucisr_output *output)
{
    IF(OgUciServerRead(lt->hucis,input,output))
    {
      int waited_time = (OgMicroClock() - lt->t0) / 1000;
      if (output->timed_out)
      {
        NlsThrowError(lt,
            "OgListeningThreadAnswerUci: timed-out in OgUciServerRead after %d milli-seconds, with timeout=%d milli-seconds",
            waited_time, input->timeout);
        DPcErr;
      }
      else
      {
        OgMsgErr(lt->hmsg, "OgUciRead", 0, 0, 0, DOgMsgSeverityError, DOgErrLogFlagNoSystemError + DOgErrLogFlagNotInErr);
        NlsThrowError(lt,
            "OgListeningThreadAnswerUci: error in OgUciServerRead after %d milli-seconds, if the error is related to \"buffer full\", set max_request_size to a larger value",
            waited_time);
        DPcErr;
      }
    }

    if (output->hh.request_method > 0 && output->hh.request_method != DOgHttpHeaderTypePost)
    {
      NlsThrowError(lt, "OgListeningThreadAnswerUci: UCI request is not an HTTP POST Request.");
      DPcErr;
    }

    if (output->content_length <= 0 || (output->content_length - output->header_length) <= 1)
    {
      NlsThrowError(lt, "OgListeningThreadAnswerUci: UCI request body is empty.");
      DPcErr;
    }

    DONE;
}

static og_status OgListeningProcessSearchRequest(struct og_listening_thread *lt, struct og_ucisw_input *winput,
    struct og_ucisr_output *output, unsigned char *top_tag)
{

#define TEST_SOCKETS
#ifdef TEST_SOCKETS
  winput->content =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<ssi_control_answers>\n"
  "  <control_answer name=\"test\">test answer successful</control_answer>\n"
  "</ssi_control_answers>\n";
  winput->content_length = strlen(winput->content);
#else

  struct og_ctrl_nls *ctrl_nls = lt->ctrl_nls;
    IFE(OgListeningThreadProcessAnswerUci(lt, output, top_tag));

    if (lt->request_in_error)
    {
      winput->http_status = 500;
      winput->http_status_message = "Internal Server Error";
    }

    // If JSON set content type
    if (NlsJsonIsEnable(lt))
    {
      winput->content_type = "application/json";
    }

    // XML and JSON output
    winput->content_length = OgHeapGetCellsUsed(lt->hbn);
    IFn(winput->content = OgHeapGetCell(lt->hbn, 0)) DPcErr;

#endif

  DONE;
}

static og_bool OgListeningAnswer(struct og_listening_thread *lt, struct og_ucisw_input *winput, struct og_ucisr_output *output, og_bool is_ssi_control_commands)
{
  lt->t2 = OgMicroClock();

  IF(OgUciServerWrite(lt->hucis,winput))
  {
    if (lt->loginfo->trace & DOgNlsTraceMinimal)
    {
      OgMsg(lt->hmsg, "", DOgMsgDestInLog + DOgMsgDestInErr,
          "OgListeningThreadAnswerUci: connexion was prematurely closed by client, going on");
    }
  }

  /** This is the only place where we need to close the socket **/
  if (lt->loginfo->trace & DOgNlsTraceSocketSize)
  {
    OgMsg(lt->hmsg, "", DOgMsgDestInLog, "OgListeningThreadAnswerUci: closing socket %d", lt->hsocket_in);
  }
  OgCloseSocket(lt->hsocket_in);

  return FALSE;
}



/*
 *  The stop message can be anything, for example:
 *    <?xml version="1.0" encoding="UTF-8"?>
 *    <control_commands>
 *      <control_command name="stop"/>
 *    </control_commands>
 *  The UCI api does not handle this as lt should
 *  not understand the content of the XML buffer.
*/

static int OgListeningReceivedStop(struct og_listening_thread *lt, struct og_ucisr_output *output,
    unsigned char *top_tag)
{
  int is = output->content_length - output->header_length;
  unsigned char *s = output->content + output->header_length;
  unsigned char *stop = "<control_command name=\"stop\"/>";
  int istop = sizeof("<control_command name=\"stop\"/>") - 1;
  int i;

  /** We check the top level tag to make avoid confusion with other request types **/
  if (Ogstricmp(top_tag, "ssi_control_commands")) return (0);

  for (i = 0; i < is; i++)
  {
    if (i + istop < is && !Ogmemicmp(s + i, stop, istop)) return (1);
  }

  return (0);
}



static int OgListeningAnswerStop(struct og_listening_thread *lt, int hsocket)
{
struct og_ucisw_input cwinput,*winput=&cwinput;
unsigned char *answer =
   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
   "<ssi_control_answers>\n"
   "  <control_answer name=\"stop\">ok</control_answer>\n"
   "</ssi_control_answers>\n";
int ianswer=strlen(answer);

memset(winput,0,sizeof(struct og_ucisw_input));
winput->content_length=ianswer;
winput->content=answer;
winput->hsocket=hsocket;

IF(OgUciServerWrite(lt->hucis,winput)) {
  if (lt->loginfo->trace & DOgNlsTraceMinimal) {
    OgMsg(lt->hmsg,"",DOgMsgDestInLog+DOgMsgDestInErr
      ,"OgUciServerWrite: connexion was prematurely closed by client on a stop command, giving up");
    }
  }

DONE;
}






