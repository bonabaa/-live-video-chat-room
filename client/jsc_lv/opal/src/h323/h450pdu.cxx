/*
 * h450pdu.cxx
 *
 * H.450 Helper functions
 *
 * Open H323 Library
 *
 * Copyright (c) 2001 Norwood Systems Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 20344 $
 * $Author: rjongbloed $
 * $Date: 2008-05-30 03:10:56 +0000 (Fri, 30 May 2008) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_H323
#if OPAL_H450

#ifdef __GNUC__
#pragma implementation "h450pdu.h"
#endif

#include <h323/h450pdu.h>

#include <h323/transaddr.h>
#include <h323/h323pdu.h>
#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <asn/h4502.h>
#include <asn/h4504.h>
#include <asn/h4506.h>
#include <asn/h4507.h>
#include <asn/h45011.h>


///////////////////////////////////////////////////////////////////////////////

X880_Invoke& H450ServiceAPDU::BuildInvoke(int invokeId, int operation)
{
  SetTag(X880_ROS::e_invoke);
  X880_Invoke& invoke = (X880_Invoke&) *this;

  invoke.m_invokeId = invokeId;

  invoke.m_opcode.SetTag(X880_Code::e_local);
  PASN_Integer& opcode = (PASN_Integer&) invoke.m_opcode;
  opcode.SetValue(operation);

  return invoke;
}


X880_ReturnResult& H450ServiceAPDU::BuildReturnResult(int invokeId)
{
  SetTag(X880_ROS::e_returnResult);
  X880_ReturnResult& returnResult = (X880_ReturnResult&) *this;

  returnResult.m_invokeId = invokeId;

  return returnResult;
}


X880_ReturnError& H450ServiceAPDU::BuildReturnError(int invokeId, int error)
{
  SetTag(X880_ROS::e_returnError);
  X880_ReturnError& returnError = (X880_ReturnError&) *this;

  returnError.m_invokeId = invokeId;

  returnError.m_errorCode.SetTag(X880_Code::e_local);
  PASN_Integer& errorCode = (PASN_Integer&) returnError.m_errorCode;
  errorCode.SetValue(error);

  return returnError;
}


X880_Reject& H450ServiceAPDU::BuildReject(int invokeId)
{
  SetTag(X880_ROS::e_reject);
  X880_Reject& reject = (X880_Reject&) *this;

  reject.m_invokeId = invokeId;

  return reject;
}


void H450ServiceAPDU::BuildCallTransferInitiate(int invokeId,
                                                const PString & callIdentity,
                                                const PString & alias,
                                                const H323TransportAddress & address)
{
  X880_Invoke& invoke = BuildInvoke(invokeId, H4502_CallTransferOperation::e_callTransferInitiate);

  H4502_CTInitiateArg argument;

  argument.m_callIdentity = callIdentity;

  H4501_ArrayOf_AliasAddress& aliasAddress = argument.m_reroutingNumber.m_destinationAddress;

  // We have to have at least a destination transport address or alias.
  if (!alias.IsEmpty() && !address.IsEmpty()) {
    aliasAddress.SetSize(2);

    // Set the alias
    aliasAddress[1].SetTag(H225_AliasAddress::e_dialedDigits);
    H323SetAliasAddress(alias, aliasAddress[1]);

    // Set the transport
    aliasAddress[0].SetTag(H225_AliasAddress::e_transportID);
    H225_TransportAddress& cPartyTransport = (H225_TransportAddress&) aliasAddress[0];
    address.SetPDU(cPartyTransport);
  }
  else {
    aliasAddress.SetSize(1);
    if (alias.IsEmpty()) {
      // Set the transport, no alias present
      aliasAddress[0].SetTag(H225_AliasAddress::e_transportID);
      H225_TransportAddress& cPartyTransport = (H225_TransportAddress&) aliasAddress[0];
      address.SetPDU(cPartyTransport);
    }
    else {
      // Set the alias, no transport
      aliasAddress[0].SetTag(H225_AliasAddress::e_dialedDigits);
      H323SetAliasAddress(alias, aliasAddress[0]);
    }
  }

  PTRACE(4, "H4502\tSending supplementary service PDU argument:\n  "
         << setprecision(2) << argument);

  invoke.IncludeOptionalField(X880_Invoke::e_argument);
  invoke.m_argument.EncodeSubType(argument);
}


void H450ServiceAPDU::BuildCallTransferSetup(int invokeId,
                                             const PString & callIdentity)
{
  X880_Invoke& invoke = BuildInvoke(invokeId, H4502_CallTransferOperation::e_callTransferSetup);

  H4502_CTSetupArg argument;

  argument.m_callIdentity = callIdentity;

  PTRACE(4, "H4502\tSending supplementary service PDU argument:\n  "
         << setprecision(2) << argument);

  invoke.IncludeOptionalField(X880_Invoke::e_argument);
  invoke.m_argument.EncodeSubType(argument);
}


void H450ServiceAPDU::BuildCallTransferIdentify(int invokeId)
{
  X880_Invoke invoke = BuildInvoke(invokeId, H4502_CallTransferOperation::e_callTransferIdentify);
}


void H450ServiceAPDU::BuildCallTransferAbandon(int invokeId)
{
  X880_Invoke invoke = BuildInvoke(invokeId, H4502_CallTransferOperation::e_callTransferAbandon);
}


void H450ServiceAPDU::BuildCallWaiting(int invokeId, int numCallsWaiting)
{
  X880_Invoke& invoke = BuildInvoke(invokeId, H4506_CallWaitingOperations::e_callWaiting);

  H4506_CallWaitingArg argument;

  argument.IncludeOptionalField(H4506_CallWaitingArg::e_nbOfAddWaitingCalls);
  argument.m_nbOfAddWaitingCalls = numCallsWaiting;

  PTRACE(4, "H4502\tSending supplementary service PDU argument:\n  "
         << setprecision(2) << argument);
  
  invoke.IncludeOptionalField(X880_Invoke::e_argument);
  invoke.m_argument.EncodeSubType(argument);
}

/* H450.7 */
void H450ServiceAPDU::BuildMessageWaiting(int invokeId, 
                                          H4507_H323_MWI_Operations  &mwiOp, 
                                          PASN_Sequence &argument)
{
  X880_Invoke& invoke = BuildInvoke(invokeId, mwiOp);
  
  PTRACE(4, "H4507\tBuildMessageWaiting: invoke " << invokeId);
  
  invoke.IncludeOptionalField(X880_Invoke::e_argument);
  invoke.m_argument.EncodeSubType(argument);
}

void H450ServiceAPDU::BuildInterrogateResult(int invokeId, 
                                             H4507_MWIInterrogateRes &interrogateResult)
{
  PTRACE(4, "H4507\tBuildInterrogateResult: invoke " << invokeId);
  X880_ReturnResult& result = BuildReturnResult(invokeId);
  result.IncludeOptionalField(X880_ReturnResult::e_result);
  result.m_result.m_opcode.SetTag(X880_Code::e_local);
  PASN_Integer& operation = (PASN_Integer&) result.m_result.m_opcode;
  operation.SetValue(H4507_H323_MWI_Operations::e_mwiInterrogate);
  
  PPER_Stream resultStream;
  interrogateResult.Encode(resultStream);
  resultStream.CompleteEncoding();
  result.m_result.m_result.SetValue(resultStream);      
  
}


void H450ServiceAPDU::BuildCallIntrusionForcedRelease(int invokeId,
                                                      int CICL)
{
  X880_Invoke& invoke = BuildInvoke(invokeId, H45011_H323CallIntrusionOperations::e_callIntrusionForcedRelease);

  H45011_CIFrcRelArg argument;

  argument.m_ciCapabilityLevel = CICL;

  invoke.IncludeOptionalField(X880_Invoke::e_argument);
  invoke.m_argument.EncodeSubType(argument);
}


X880_ReturnResult& H450ServiceAPDU::BuildCallIntrusionForcedReleaseResult(int invokeId)
{
  PTRACE(3 ,"H450.11\tH450ServiceAPDU::BuildCallIntrusionForcedReleaseResult BEGIN");
  
  X880_ReturnResult& result = BuildReturnResult(invokeId);
  result.IncludeOptionalField(X880_ReturnResult::e_result);
  result.m_result.m_opcode.SetTag(X880_Code::e_local);
  PASN_Integer& operation = (PASN_Integer&) result.m_result.m_opcode;
  operation.SetValue(H45011_H323CallIntrusionOperations::e_callIntrusionForcedRelease);

  H45011_CIFrcRelOptRes ciCIPLRes;

  PPER_Stream resultStream;
  ciCIPLRes.Encode(resultStream);
  resultStream.CompleteEncoding();
  result.m_result.m_result.SetValue(resultStream);
  PTRACE(4 ,"H450.11\tH450ServiceAPDU::BuildCallIntrusionForcedReleaseResult END");

  return result;
}


void H450ServiceAPDU::BuildCallIntrusionForcedReleaseError()
{
/**
  TBD
*/
}


void H450ServiceAPDU::BuildCallIntrusionGetCIPL(int invokeId)
{
  PTRACE(4, "H450.11\tBuildCallIntrusionGetCIPL invokeId=" << invokeId);
  X880_Invoke invoke = BuildInvoke(invokeId, H45011_H323CallIntrusionOperations::e_callIntrusionGetCIPL);
}


void H450ServiceAPDU::BuildCallIntrusionImpending(int invokeId)
{
  PTRACE(4, "H450.11\tBuildCallIntrusionImpending invokeId=" << invokeId);
  X880_Invoke& invoke = BuildInvoke(invokeId, H45011_H323CallIntrusionOperations::e_callIntrusionNotification);
 
  H45011_CINotificationArg argument;

  argument.m_ciStatusInformation = H45011_CIStatusInformation::e_callIntrusionImpending;

  invoke.IncludeOptionalField(X880_Invoke::e_argument);
  invoke.m_argument.EncodeSubType(argument);
}


void H450ServiceAPDU::BuildCallIntrusionForceRelesed(int invokeId)
{
  PTRACE(4, "H450.11\tBuildCallIntrusionForceRelesed invokeId=" << invokeId);
  X880_Invoke& invoke = BuildInvoke(invokeId, H45011_H323CallIntrusionOperations::e_callIntrusionNotification);
 
  H45011_CINotificationArg argument;

  argument.m_ciStatusInformation = H45011_CIStatusInformation::e_callForceReleased;

  invoke.IncludeOptionalField(X880_Invoke::e_argument);
  invoke.m_argument.EncodeSubType(argument);
}


void H450ServiceAPDU::AttachSupplementaryServiceAPDU(H323SignalPDU & pdu)
{
  H4501_SupplementaryService supplementaryService;

  // Create an H.450.1 supplementary service object
  // and store the H450ServiceAPDU in the ROS array.
  supplementaryService.m_serviceApdu.SetTag(H4501_ServiceApdus::e_rosApdus);
  H4501_ArrayOf_ROS & operations = (H4501_ArrayOf_ROS &)supplementaryService.m_serviceApdu;
  operations.SetSize(1);
  operations[0] = *this;

  PTRACE(4, "H4501\tSending supplementary service PDU:\n  "
         << setprecision(2) << supplementaryService);

  // Add the H.450 PDU to the H.323 User-to-User PDU as an OCTET STRING
  pdu.m_h323_uu_pdu.IncludeOptionalField(H225_H323_UU_PDU::e_h4501SupplementaryService);
  pdu.m_h323_uu_pdu.m_h4501SupplementaryService.SetSize(1);
  pdu.m_h323_uu_pdu.m_h4501SupplementaryService[0].EncodeSubType(supplementaryService);
}


PBoolean H450ServiceAPDU::WriteFacilityPDU(H323Connection & connection)
{
  H323SignalPDU facilityPDU;
  facilityPDU.BuildFacility(connection, PTrue);

  AttachSupplementaryServiceAPDU(facilityPDU);

  PSafeLockReadOnly lock(connection);
  return connection.WriteSignalPDU(facilityPDU);
}


void H450ServiceAPDU::ParseEndpointAddress(H4501_EndpointAddress& endpointAddress,
                                           PString& remoteParty)
{
  H4501_ArrayOf_AliasAddress& destinationAddress = endpointAddress.m_destinationAddress;

  PString alias;
  H323TransportAddress transportAddress;

  for (PINDEX i = 0; i < destinationAddress.GetSize(); i++) {
    H225_AliasAddress& aliasAddress = destinationAddress[i];

    if (aliasAddress.GetTag() == H225_AliasAddress::e_transportID)
      transportAddress = (H225_TransportAddress &)aliasAddress;
    else
      alias = ::H323GetAliasAddressString(aliasAddress);
  }

  if (alias.IsEmpty()) {
    remoteParty = transportAddress;
  }
  else if (transportAddress.IsEmpty()) {
    remoteParty = alias;
  }
  else {
    remoteParty = alias + '@' + transportAddress;
  }
}


/////////////////////////////////////////////////////////////////////////////

H450xDispatcher::H450xDispatcher(H323Connection & conn)
  : connection(conn)
{
  opcodeHandler.DisallowDeleteObjects();

  nextInvokeId = 0;
}


void H450xDispatcher::AddOpCode(unsigned opcode, H450xHandler * handler)
{
  if (PAssertNULL(handler) == NULL)
    return;

  if (handlers.GetObjectsIndex(handler) == P_MAX_INDEX)
    handlers.Append(handler);

  opcodeHandler.SetAt(opcode, handler);
}


void H450xDispatcher::AttachToSetup(H323SignalPDU & pdu)
{
  for (PINDEX i = 0; i < handlers.GetSize(); i++)
    handlers[i].AttachToSetup(pdu);
}


void H450xDispatcher::AttachToAlerting(H323SignalPDU & pdu)
{
  for (PINDEX i = 0; i < handlers.GetSize(); i++)
    handlers[i].AttachToAlerting(pdu);
}


void H450xDispatcher::AttachToConnect(H323SignalPDU & pdu)
{
  for (PINDEX i = 0; i < handlers.GetSize(); i++)
    handlers[i].AttachToConnect(pdu);
}


void H450xDispatcher::AttachToReleaseComplete(H323SignalPDU & pdu)
{
  for (PINDEX i = 0; i < handlers.GetSize(); i++)
    handlers[i].AttachToReleaseComplete(pdu);
}


PBoolean H450xDispatcher::HandlePDU(const H323SignalPDU & pdu)
{
PBoolean result = PTrue;
  for (PINDEX i = 0; i < pdu.m_h323_uu_pdu.m_h4501SupplementaryService.GetSize(); i++) {
    H4501_SupplementaryService supplementaryService;

    // Decode the supplementary service PDU from the PPER Stream
    if (pdu.m_h323_uu_pdu.m_h4501SupplementaryService[i].DecodeSubType(supplementaryService)) {
      PTRACE(4, "H4501\tReceived supplementary service PDU:\n  "
             << setprecision(2) << supplementaryService);
    }
    else {
      PTRACE(1, "H4501\tInvalid supplementary service PDU decode:\n  "
             << setprecision(2) << supplementaryService);
      continue;
    }

    H4501_InterpretationApdu & interpretation = supplementaryService.m_interpretationApdu;

    if (supplementaryService.m_serviceApdu.GetTag() == H4501_ServiceApdus::e_rosApdus) {
      H4501_ArrayOf_ROS& operations = (H4501_ArrayOf_ROS&) supplementaryService.m_serviceApdu;

      for (PINDEX j = 0; j < operations.GetSize(); j ++) {
        X880_ROS& operation = operations[j];

        PTRACE(3, "H4501\tX880 ROS " << operation.GetTagName());

        switch (operation.GetTag()) {
          case X880_ROS::e_invoke:
            result = OnReceivedInvoke((X880_Invoke &)operation, interpretation);
            break;

          case X880_ROS::e_returnResult:
            result = OnReceivedReturnResult((X880_ReturnResult &)operation);
            break;

          case X880_ROS::e_returnError:
            result = OnReceivedReturnError((X880_ReturnError &)operation);
            break;

          case X880_ROS::e_reject:
            result = OnReceivedReject((X880_Reject &)operation);
            break;

          default :
            break;
        }
      }
    }
  }
  return result;
}


PBoolean H450xDispatcher::OnReceivedInvoke(X880_Invoke & invoke, H4501_InterpretationApdu & interpretation)
{
  PBoolean result = PTrue;
  // Get the invokeId
  int invokeId = invoke.m_invokeId.GetValue();

  // Get the linkedId if present
  int linkedId = -1;
  if (invoke.HasOptionalField(X880_Invoke::e_linkedId)) {
    linkedId = invoke.m_linkedId.GetValue();
  }

  // Get the argument if present
  PASN_OctetString * argument = NULL;
  if (invoke.HasOptionalField(X880_Invoke::e_argument)) {
    argument = &invoke.m_argument;
  }

  // Get the opcode
  if (invoke.m_opcode.GetTag() == X880_Code::e_local) {
    int opcode = ((PASN_Integer&) invoke.m_opcode).GetValue();
    if (!opcodeHandler.Contains(opcode)) {
      PTRACE(3, "H4501\tInvoke of unsupported local opcode:\n  " << invoke);	  
      if (interpretation.GetTag() != H4501_InterpretationApdu::e_discardAnyUnrecognizedInvokePdu)
        SendInvokeReject(invokeId, 1 /*X880_InvokeProblem::e_unrecognisedOperation*/);
      if (interpretation.GetTag() == H4501_InterpretationApdu::e_clearCallIfAnyInvokePduNotRecognized)
        result = PFalse;
    }
    else
      result = opcodeHandler[opcode].OnReceivedInvoke(opcode, invokeId, linkedId, argument);
  }
  else {
    if (interpretation.GetTag() != H4501_InterpretationApdu::e_discardAnyUnrecognizedInvokePdu)
      SendInvokeReject(invokeId, 1 /*X880_InvokeProblem::e_unrecognisedOperation*/);
    PTRACE(2, "H4501\tInvoke of unsupported global opcode:\n  " << invoke);
    if (interpretation.GetTag() == H4501_InterpretationApdu::e_clearCallIfAnyInvokePduNotRecognized)
      result = PFalse;
  }
  return result;
}


PBoolean H450xDispatcher::OnReceivedReturnResult(X880_ReturnResult & returnResult)
{
  unsigned invokeId = returnResult.m_invokeId.GetValue();

  for (PINDEX i = 0; i < handlers.GetSize(); i++) {
    if (handlers[i].GetInvokeId() == invokeId) {
      handlers[i].OnReceivedReturnResult(returnResult);
      break;
    }
  }
  return PTrue;
}


PBoolean H450xDispatcher::OnReceivedReturnError(X880_ReturnError & returnError)
{
  PBoolean result=PTrue;
  unsigned invokeId = returnError.m_invokeId.GetValue();
  int errorCode = 0;

  if (returnError.m_errorCode.GetTag() == X880_Code::e_local)
    errorCode = ((PASN_Integer&) returnError.m_errorCode).GetValue();

  for (PINDEX i = 0; i < handlers.GetSize(); i++) {
    if (handlers[i].GetInvokeId() == invokeId) {
      result = handlers[i].OnReceivedReturnError(errorCode, returnError);
      break;
    }
  }
  return result;
}


PBoolean H450xDispatcher::OnReceivedReject(X880_Reject & reject)
{
  int problem = 0;

  switch (reject.m_problem.GetTag()) {
    case X880_Reject_problem::e_general:
    {
      X880_GeneralProblem & generalProblem = reject.m_problem;
      problem = generalProblem.GetValue();
    }
    break;

    case X880_Reject_problem::e_invoke:
    {
      X880_InvokeProblem & invokeProblem = reject.m_problem;
      problem = invokeProblem.GetValue();
    }
    break;

    case X880_Reject_problem::e_returnResult:
    {
      X880_ReturnResultProblem & returnResultProblem = reject.m_problem;
      problem = returnResultProblem.GetValue();
    }
    break;

    case X880_Reject_problem::e_returnError:
    {
      X880_ReturnErrorProblem & returnErrorProblem = reject.m_problem;
      problem = returnErrorProblem.GetValue();
    }
    break;

    default:
      break;
  }


  unsigned invokeId = reject.m_invokeId;
  for (PINDEX i = 0; i < handlers.GetSize(); i++) {
    if (handlers[i].GetInvokeId() == invokeId) {
      handlers[i].OnReceivedReject(reject.m_problem.GetTag(), problem);
      break;
    }
  }
  return PTrue;
}


void H450xDispatcher::SendReturnError(int invokeId, int returnError)
{
  H450ServiceAPDU serviceAPDU;

  serviceAPDU.BuildReturnError(invokeId, returnError);

  serviceAPDU.WriteFacilityPDU(connection);
}


void H450xDispatcher::SendGeneralReject(int invokeId, int problem)
{
  H450ServiceAPDU serviceAPDU;

  X880_Reject & reject = serviceAPDU.BuildReject(invokeId);
  reject.m_problem.SetTag(X880_Reject_problem::e_general);
  X880_GeneralProblem & generalProblem = (X880_GeneralProblem &) reject.m_problem;
  generalProblem = problem;

  serviceAPDU.WriteFacilityPDU(connection);
}


void H450xDispatcher::SendInvokeReject(int invokeId, int problem)
{
  H450ServiceAPDU serviceAPDU;

  X880_Reject & reject = serviceAPDU.BuildReject(invokeId);
  reject.m_problem.SetTag(X880_Reject_problem::e_invoke);
  X880_InvokeProblem & invokeProblem = (X880_InvokeProblem &) reject.m_problem;
  invokeProblem = problem;

  serviceAPDU.WriteFacilityPDU(connection);
}


void H450xDispatcher::SendReturnResultReject(int invokeId, int problem)
{
  H450ServiceAPDU serviceAPDU;

  X880_Reject & reject = serviceAPDU.BuildReject(invokeId);
  reject.m_problem.SetTag(X880_Reject_problem::e_returnResult);
  X880_ReturnResultProblem & returnResultProblem = reject.m_problem;
  returnResultProblem = problem;

  serviceAPDU.WriteFacilityPDU(connection);
}


void H450xDispatcher::SendReturnErrorReject(int invokeId, int problem)
{
  H450ServiceAPDU serviceAPDU;

  X880_Reject & reject = serviceAPDU.BuildReject(invokeId);
  reject.m_problem.SetTag(X880_Reject_problem::e_returnError);
  X880_ReturnErrorProblem & returnErrorProblem = reject.m_problem;
  returnErrorProblem = problem;

  serviceAPDU.WriteFacilityPDU(connection);
}


/////////////////////////////////////////////////////////////////////////////

H450xHandler::H450xHandler(H323Connection & conn, H450xDispatcher & disp)
  : endpoint(conn.GetEndPoint()),
    connection(conn),
    dispatcher(disp)
{
  currentInvokeId = 0;
}


void H450xHandler::AttachToSetup(H323SignalPDU &)
{
}


void H450xHandler::AttachToAlerting(H323SignalPDU &)
{
}


void H450xHandler::AttachToConnect(H323SignalPDU &)
{
}


void H450xHandler::AttachToReleaseComplete(H323SignalPDU &)
{
}


PBoolean H450xHandler::OnReceivedReturnResult(X880_ReturnResult & /*returnResult*/)
{
  return PTrue;
}


PBoolean H450xHandler::OnReceivedReturnError(int /*errorCode*/,
                                        X880_ReturnError & /*returnError*/)
{
  return PTrue;
}


PBoolean H450xHandler::OnReceivedReject(int /*problemType*/,
                                   int /*problemNumber*/)
{
  return PTrue;
}


void H450xHandler::SendReturnError(int returnError)
{
  dispatcher.SendReturnError(currentInvokeId, returnError);
  currentInvokeId = 0;
}


void H450xHandler::SendGeneralReject(int problem)
{
  dispatcher.SendGeneralReject(currentInvokeId, problem);
  currentInvokeId = 0;
}


void H450xHandler::SendInvokeReject(int problem)
{
  dispatcher.SendInvokeReject(currentInvokeId, problem);
  currentInvokeId = 0;
}


void H450xHandler::SendReturnResultReject(int problem)
{
  dispatcher.SendReturnResultReject(currentInvokeId, problem);
  currentInvokeId = 0;
}


void H450xHandler::SendReturnErrorReject(int problem)
{
  dispatcher.SendReturnErrorReject(currentInvokeId, problem);
  currentInvokeId = 0;
}


PBoolean H450xHandler::DecodeArguments(PASN_OctetString * argString,
                                  PASN_Object & argObject,
                                  int absentErrorCode)
{
  if (argString == NULL) {
    if (absentErrorCode >= 0)
      SendReturnError(absentErrorCode);
    return PFalse;
  }

  PPER_Stream argStream(*argString);
  if (argObject.Decode(argStream)) {
    PTRACE(4, "H4501\tSupplementary service argument:\n  "
           << setprecision(2) << argObject);
    return PTrue;
  }

  PTRACE(1, "H4501\tInvalid supplementary service argument:\n  "
         << setprecision(2) << argObject);
  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////

H4502Handler::H4502Handler(H323Connection & conn, H450xDispatcher & disp)
  : H450xHandler(conn, disp)
{
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferIdentify, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferAbandon, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferInitiate, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferSetup, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferUpdate, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_subaddressTransfer, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferComplete, this);
  dispatcher.AddOpCode(H4502_CallTransferOperation::e_callTransferActive, this);

  transferringCallToken = "";
  ctState = e_ctIdle;
  ctResponseSent = PFalse;
  CallToken = PString();
  consultationTransfer = PFalse;

  ctTimer.SetNotifier(PCREATE_NOTIFIER(OnCallTransferTimeOut));
}


void H4502Handler::AttachToSetup(H323SignalPDU & pdu)
{
  // Do we need to attach a call transfer setup invoke APDU?
  if (ctState != e_ctAwaitSetupResponse)
    return;

  H450ServiceAPDU serviceAPDU;

  // Store the outstanding invokeID associated with this connection
  currentInvokeId = dispatcher.GetNextInvokeId();

  // Use the call identity from the ctInitiateArg
  serviceAPDU.BuildCallTransferSetup(currentInvokeId, transferringCallIdentity);

  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
}


void H4502Handler::AttachToAlerting(H323SignalPDU & pdu)
{
  // Do we need to send a callTransferSetup return result APDU?
  if (currentInvokeId == 0 || ctResponseSent)
    return;

  H450ServiceAPDU serviceAPDU;
  serviceAPDU.BuildReturnResult(currentInvokeId);
  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
  ctResponseSent = PTrue;
  currentInvokeId = 0;
}


void H4502Handler::AttachToConnect(H323SignalPDU & pdu)
{
  // Do we need to include a ctInitiateReturnResult APDU in our Release Complete Message?
  if (currentInvokeId == 0 || ctResponseSent)
    return;

  H450ServiceAPDU serviceAPDU;
  serviceAPDU.BuildReturnResult(currentInvokeId);
  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
  ctResponseSent = PTrue;
  currentInvokeId = 0;
}


void H4502Handler::AttachToReleaseComplete(H323SignalPDU & pdu)
{
  // Do we need to include a ctInitiateReturnResult APDU in our Release Complete Message?
  if (currentInvokeId == 0)
    return;

  // If the SETUP message we received from the other end had a callTransferSetup APDU
  // in it, then we need to send back a RELEASE COMPLETE PDU with a callTransferSetup 
  // ReturnError.
  // Else normal call - clear it down
  H450ServiceAPDU serviceAPDU;

  if (ctResponseSent) {
    serviceAPDU.BuildReturnResult(currentInvokeId);
    ctResponseSent = PFalse;
    currentInvokeId = 0;
  }
  else {
    serviceAPDU.BuildReturnError(currentInvokeId, H4501_GeneralErrorList::e_notAvailable);
    ctResponseSent = PTrue;
    currentInvokeId = 0;
  }

  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
}


PBoolean H4502Handler::OnReceivedInvoke(int opcode,
                                    int invokeId,
                                    int linkedId,
                                    PASN_OctetString * argument)
{
  currentInvokeId = invokeId;

  switch (opcode) {
    case H4502_CallTransferOperation::e_callTransferIdentify:
      OnReceivedCallTransferIdentify(linkedId);
      break;

    case H4502_CallTransferOperation::e_callTransferAbandon:
      OnReceivedCallTransferAbandon(linkedId);
      break;

    case H4502_CallTransferOperation::e_callTransferInitiate:
      OnReceivedCallTransferInitiate(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_callTransferSetup:
      OnReceivedCallTransferSetup(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_callTransferUpdate:
      OnReceivedCallTransferUpdate(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_subaddressTransfer:
      OnReceivedSubaddressTransfer(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_callTransferComplete:
      OnReceivedCallTransferComplete(linkedId, argument);
      break;

    case H4502_CallTransferOperation::e_callTransferActive:
      OnReceivedCallTransferActive(linkedId, argument);
      break;

    default:
      currentInvokeId = 0;
      return PFalse;
  }

  return PTrue;
}


void H4502Handler::OnReceivedCallTransferIdentify(int /*linkedId*/)
{
  if (!endpoint.OnCallTransferIdentify(connection))
    SendReturnError(H4501_GeneralErrorList::e_notAvailable);
  
  // Send a FACILITY message with a callTransferIdentify return result
  // Supplementary Service PDU to the transferring endpoint.
  H450ServiceAPDU serviceAPDU;

  X880_ReturnResult& result = serviceAPDU.BuildReturnResult(currentInvokeId);
  result.IncludeOptionalField(X880_ReturnResult::e_result);
  result.m_result.m_opcode.SetTag(X880_Code::e_local);
  PASN_Integer& operation = (PASN_Integer&) result.m_result.m_opcode;
  operation.SetValue(H4502_CallTransferOperation::e_callTransferIdentify);

  H4502_CTIdentifyRes ctIdentifyResult;

  // Restrict the generated value to 4 digits (13 bits)
  unsigned int id = endpoint.GetNextH450CallIdentityValue() & 0x1FFF;
  PString pstrId(PString::Unsigned, id);
  ctIdentifyResult.m_callIdentity = pstrId;

  // Store the callIdentity of this connection in the dictionary
  endpoint.GetCallIdentityDictionary().SetAt(pstrId, &connection);

  H4501_ArrayOf_AliasAddress& aliasAddress = ctIdentifyResult.m_reroutingNumber.m_destinationAddress;

  PString localName = connection.GetLocalPartyName();
  if (localName.IsEmpty()) 
    aliasAddress.SetSize(1);
  else {
    aliasAddress.SetSize(2);
    aliasAddress[1].SetTag(H225_AliasAddress::e_dialedDigits);
    H323SetAliasAddress(localName, aliasAddress[1]);  // Will encode as h323-Id if not E.164
  }

  H323TransportAddress address;
  address = connection.GetSignallingChannel()->GetLocalAddress();

  aliasAddress[0].SetTag(H225_AliasAddress::e_transportID);
  H225_TransportAddress& cPartyTransport = (H225_TransportAddress&) aliasAddress[0];
  address.SetPDU(cPartyTransport, endpoint.GetDefaultSignalPort());

  PPER_Stream resultStream;
  ctIdentifyResult.Encode(resultStream);
  resultStream.CompleteEncoding();
  result.m_result.m_result.SetValue(resultStream);

  serviceAPDU.WriteFacilityPDU(connection);

  ctState = e_ctAwaitSetup;

  // start timer CT-T2
  PTRACE(4, "H450.2\tStarting timer CT-T2");
  StartctTimer(endpoint.GetCallTransferT2());
}


void H4502Handler::OnReceivedCallTransferAbandon(int /*linkedId*/)
{
  switch (ctState) {
    case e_ctAwaitSetup:
      {
        // Stop Timer CT-T2 and enter state e_ctIdle
        StopctTimer();
        PTRACE(4, "H4502\tStopping timer CT-T2");

        currentInvokeId = 0;
        ctState = e_ctIdle;
      }
      break;

    default:
      break;
  }
}


void H4502Handler::OnReceivedCallTransferInitiate(int /*linkedId*/,
                                                  PASN_OctetString * argument)
{
  // TBD: Check Call Hold status. If call is held, it must first be 
  // retrieved before being transferred. -- dcassel 4/01

  H4502_CTInitiateArg ctInitiateArg;
  if (!DecodeArguments(argument, ctInitiateArg,
                       H4502_CallTransferErrors::e_invalidReroutingNumber))
    return;

  ctResponseSent = PTrue;

  PString remoteParty;
  H450ServiceAPDU::ParseEndpointAddress(ctInitiateArg.m_reroutingNumber, remoteParty);

  if (!endpoint.OnCallTransferInitiate(connection, remoteParty) ||
      !endpoint.SetupTransfer(connection.GetCallToken(),
                              ctInitiateArg.m_callIdentity.GetValue(),
                              remoteParty))
    SendReturnError(H4502_CallTransferErrors::e_establishmentFailure);
}


void H4502Handler::OnReceivedCallTransferSetup(int /*linkedId*/,
                                               PASN_OctetString * argument)
{
  H4502_CTSetupArg ctSetupArg;
  if (!DecodeArguments(argument, ctSetupArg,
                       H4502_CallTransferErrors::e_unrecognizedCallIdentity))
    return;

  // Get the Transferring User's details if present
  PString transferringParty;
  if (ctSetupArg.HasOptionalField(H4502_CTSetupArg::e_transferringNumber)) {
    H450ServiceAPDU::ParseEndpointAddress(ctSetupArg.m_transferringNumber, transferringParty);
  }

  PString callIdentity;
  callIdentity = ctSetupArg.m_callIdentity;

  if (callIdentity.IsEmpty()) { // Blind Transfer
    switch (ctState) {
      case e_ctIdle:
        ctState = e_ctAwaitSetupResponse;
        break;

      // Wrong State
      default :
        break;
    }
  }
  else { // Transfer through Consultation
    
    // We need to check that the call identity and destination address information match those in the 
    // second call.  For the time being we just check that the call identities match as there does not 
    // appear to be an elegant solution to compare the destination address information.

    // Get this callIdentity from our dictionary (if present)
    H323Connection *secondaryCall = endpoint.GetCallIdentityDictionary().GetAt(callIdentity);
  
    if (secondaryCall != NULL)
      secondaryCall->HandleConsultationTransfer(callIdentity, connection);
    else  // Mismatched callIdentity
      SendReturnError(H4502_CallTransferErrors::e_unrecognizedCallIdentity);
  }
}


void H4502Handler::OnReceivedCallTransferUpdate(int /*linkedId*/,
                                                PASN_OctetString * argument)
{
  H4502_CTUpdateArg ctUpdateArg;
  if (!DecodeArguments(argument, ctUpdateArg, -1))
    return;

}


void H4502Handler::OnReceivedSubaddressTransfer(int /*linkedId*/,
                                                PASN_OctetString * argument)
{
  H4502_SubaddressTransferArg subaddressTransferArg;
  if (!DecodeArguments(argument, subaddressTransferArg, -1))
    return;

}


void H4502Handler::OnReceivedCallTransferComplete(int /*linkedId*/,
                                                  PASN_OctetString * argument)
{
  H4502_CTCompleteArg ctCompleteArg;
  if (!DecodeArguments(argument, ctCompleteArg, -1))
    return;

}


void H4502Handler::OnReceivedCallTransferActive(int /*linkedId*/,
                                                PASN_OctetString * argument)
{
  H4502_CTActiveArg ctActiveArg;
  if (!DecodeArguments(argument, ctActiveArg, -1))
    return;

}


PBoolean H4502Handler::OnReceivedReturnResult(X880_ReturnResult & returnResult)
{
  if (currentInvokeId == returnResult.m_invokeId.GetValue()) {
    switch (ctState) {
      case e_ctAwaitInitiateResponse:
        OnReceivedInitiateReturnResult(); 
        break;

      case e_ctAwaitSetupResponse:
        OnReceivedSetupReturnResult();
        break;

      case e_ctAwaitIdentifyResponse:
        OnReceivedIdentifyReturnResult(returnResult);
        break;

      default :
        break;
    }
  }
  return PTrue;
}


void H4502Handler::OnReceivedInitiateReturnResult()
{
  // stop timer CT-T3
  StopctTimer();
  PTRACE(4, "H4502\tStopping timer CT-T3");
  ctState = e_ctIdle;
  currentInvokeId = 0;

  // clear the primary and secondary call if not already cleared,
}


void H4502Handler::OnReceivedSetupReturnResult()
{
  // stop timer CT-T4
  StopctTimer();
  PTRACE(4, "H4502\tStopping timer CT-T4");
  ctState = e_ctIdle;
  currentInvokeId = 0;

  // Clear the primary call
  endpoint.ClearCall(transferringCallToken, H323Connection::EndedByCallForwarded);
}


void H4502Handler::OnReceivedIdentifyReturnResult(X880_ReturnResult &returnResult)
{
  // stop timer CT-T1
  StopctTimer();
  PTRACE(4, "H4502\tStopping timer CT-T1");

  // Have received response.
  ctState = e_ctIdle;

  // Get the return result if present
  PASN_OctetString * result = NULL;
  if (returnResult.HasOptionalField(X880_ReturnResult::e_result)) {
    result = &returnResult.m_result.m_result;

    // Extract the C Party Details
    H4502_CTIdentifyRes ctIdentifyResult;

    PPER_Stream resultStream(*result);
    ctIdentifyResult.Decode(resultStream);
    PString callIdentity = ctIdentifyResult.m_callIdentity.GetValue();

    PString remoteParty;
    H450ServiceAPDU::ParseEndpointAddress(ctIdentifyResult.m_reroutingNumber, remoteParty);

    // Store the secondary call token on the primary connection so we can send a 
    // callTransferAbandon invoke APDU on the secondary call at a later stage if we 
    // get back a callTransferInitiateReturnError
    PSafePtr<H323Connection> primaryConnection = endpoint.FindConnectionWithLock(CallToken);
    if (primaryConnection != NULL) {
      primaryConnection->SetAssociatedCallToken(connection.GetCallToken());

      // Send a callTransferInitiate invoke APDU in a FACILITY message
      // to the transferred endpoint on the primary call
      endpoint.TransferCall(primaryConnection->GetCallToken(), remoteParty, callIdentity);
    }
  }
}


PBoolean H4502Handler::OnReceivedReturnError(int errorCode, X880_ReturnError &returnError)
{
  if (currentInvokeId == returnError.m_invokeId.GetValue()) {
    switch (ctState) {
      case e_ctAwaitInitiateResponse:
        OnReceivedInitiateReturnError();
        break;

      case e_ctAwaitSetupResponse:
        OnReceivedSetupReturnError(errorCode);
        break;

      case e_ctAwaitIdentifyResponse:
        OnReceivedIdentifyReturnError();
        break;

      default :
        break;
    }
  }
  return PTrue;
}


void H4502Handler::OnReceivedInitiateReturnError(const bool timerExpiry)
{ 
  if (!timerExpiry) {
    // stop timer CT-T3
    StopctTimer();
    PTRACE(4, "H4502\tStopping timer CT-T3");
  }
  else
    PTRACE(4, "H4502\tTimer CT-T3 has expired on the Transferring Endpoint awaiting a response to a callTransferInitiate APDU.");

  currentInvokeId = 0;
  ctState = e_ctIdle;


  // Send a callTransferAbandon invoke APDU in a FACILITY message on the secondary call
  // (if it exists) and enter state CT-Idle.
  PSafePtr<H323Connection> secondaryConnection = endpoint.FindConnectionWithLock(CallToken, PSafeReadOnly);
  if (secondaryConnection != NULL) {
    H450ServiceAPDU serviceAPDU;
    serviceAPDU.BuildCallTransferAbandon(dispatcher.GetNextInvokeId());
    serviceAPDU.WriteFacilityPDU(*secondaryConnection);
  }

  // Notify the callTransferInitiate return error to the endpoint
  endpoint.OnReceivedInitiateReturnError();
}


void H4502Handler::OnReceivedSetupReturnError(int errorCode,
                                              const bool timerExpiry)
{
  ctState = e_ctIdle;
  currentInvokeId = 0;
  
  if (!timerExpiry) {
    // stop timer CT-T4 if it is running
    StopctTimer();
    PTRACE(4, "H4502\tStopping timer CT-T4");  
  }
  else {
    PTRACE(3, "H4502\tTimer CT-T4 has expired on the Transferred Endpoint awaiting a response to a callTransferSetup APDU.");

    // Clear the transferred call.
    endpoint.ClearCall(connection.GetCallToken());
  }

  // Send a facility message to the transferring endpoint
  // containing a call transfer initiate return error
  PSafePtr<H323Connection> primaryConnection = endpoint.FindConnectionWithLock(transferringCallToken);
  if (primaryConnection != NULL)
    primaryConnection->HandleCallTransferFailure(errorCode);
}


void H4502Handler::OnReceivedIdentifyReturnError(const bool timerExpiry)
{
  // The transferred-to user cannot participate in our transfer request
  ctState = e_ctIdle;
  currentInvokeId = 0;

  if (!timerExpiry) {
    // stop timer CT-T1
    StopctTimer();
    PTRACE(4, "H4502\tStopping timer CT-T1");
  }
  else {
    PTRACE(4, "H4502\tTimer CT-T1 has expired on the Transferring Endpoint awaiting a response to a callTransferIdentify APDU.");

    // send a callTransferAbandon invoke APDU in a FACILITY message on the secondary call
    // and enter state CT-Idle.
    H450ServiceAPDU serviceAPDU;

    serviceAPDU.BuildCallTransferAbandon(dispatcher.GetNextInvokeId());
    serviceAPDU.WriteFacilityPDU(connection);
  }
}

bool H4502Handler::TransferCall(const PString & remoteParty,
                                const PString & callIdentity)
{
  currentInvokeId = dispatcher.GetNextInvokeId();

  // Send a FACILITY message with a callTransferInitiate Invoke
  // Supplementary Service PDU to the transferred endpoint.
  H450ServiceAPDU serviceAPDU;

  PString alias;
  H323TransportAddress address;
  if (!endpoint.ParsePartyName(remoteParty, alias, address))
    return false;

  serviceAPDU.BuildCallTransferInitiate(currentInvokeId, callIdentity, alias, address);
  if (!serviceAPDU.WriteFacilityPDU(connection))
    return false;

  ctState = e_ctAwaitInitiateResponse;

  // start timer CT-T3
  PTRACE(4, "H4502\tStarting timer CT-T3");
  StartctTimer(connection.GetEndPoint().GetCallTransferT3());
  return true;
}


void H4502Handler::ConsultationTransfer(const PString & primaryCallToken)
{
  currentInvokeId = dispatcher.GetNextInvokeId();

  // Store the call token of the primary call on the secondary call.
  SetAssociatedCallToken(primaryCallToken);

  // Send a FACILITY message with a callTransferIdentify Invoke
  // Supplementary Service PDU to the transferred-to endpoint.
  H450ServiceAPDU serviceAPDU;

  serviceAPDU.BuildCallTransferIdentify(currentInvokeId);
  serviceAPDU.WriteFacilityPDU(connection);

  ctState = e_ctAwaitIdentifyResponse;

  // start timer CT-T1
  PTRACE(4, "H4502\tStarting timer CT-T1");
  StartctTimer(endpoint.GetCallTransferT1());
}


void H4502Handler::HandleConsultationTransfer(const PString & callIdentity,
                                              H323Connection& incoming)
{
  switch (ctState) {
    case e_ctAwaitSetup:
      {
        // Remove this callIdentity, connection pair from our dictionary as we no longer need it
        endpoint.GetCallIdentityDictionary().RemoveAt(callIdentity);

        // Stop timer CT-T2
        StopctTimer();
        PTRACE(4, "H4502\tStopping timer CT-T2");

        PTRACE(4, "H450.2\tConsultation Transfer successful, clearing secondary call");

        incoming.OnConsultationTransferSuccess(connection);

        currentInvokeId = 0;
        ctState = e_ctIdle;

        endpoint.ClearCall(connection.GetCallToken());
      }
      break;

    // Wrong Call Transfer State
    default :
      break;

  }
}


void H4502Handler::AwaitSetupResponse(const PString & token,
                                      const PString & identity)
{
  transferringCallToken = token;
  transferringCallIdentity = identity;
  ctState = e_ctAwaitSetupResponse;

  // start timer CT-T4
  PTRACE(4, "H450.2\tStarting timer CT-T4");
  StartctTimer(connection.GetEndPoint().GetCallTransferT4());
}


void H4502Handler::onReceivedAdmissionReject(const int returnError)
{
  if (ctState == e_ctAwaitSetupResponse) {
    ctState = e_ctIdle;

    // Stop timer CT-T4 if it is running
    StopctTimer();
    PTRACE(3, "H4502\tStopping timer CT-T4");

    // Send a FACILITY message back to the transferring party on the primary connection
    PSafePtr<H323Connection> primaryConnection = endpoint.FindConnectionWithLock(transferringCallToken);
    if (primaryConnection != NULL) {
      PTRACE(2, "H4502\tReceived an Admission Reject at the Transferred Endpoint - aborting the transfer.");
      primaryConnection->HandleCallTransferFailure(returnError);
    }
  }
}


void H4502Handler::HandleCallTransferFailure(const int returnError)
{
  SendReturnError(returnError);
}


void H4502Handler::StopctTimer()
{
  if (ctTimer.IsRunning())
    ctTimer.Stop();
}


void H4502Handler::OnCallTransferTimeOut(PTimer &, INT)
{
  switch (ctState) {
    // CT-T3 Timeout
    case e_ctAwaitInitiateResponse:
      OnReceivedInitiateReturnError(true);
      break;

    // CT-T1 Timeout
    case e_ctAwaitIdentifyResponse:
      OnReceivedIdentifyReturnError(true);
      break;

    // CT-T2 Timeout
    case e_ctAwaitSetup:
      {
        // Abort the call transfer
        ctState = e_ctIdle;
        currentInvokeId = 0;
        PTRACE(4, "H450.2\tTimer CT-T2 has expired on the Transferred-to endpoint awaiting a callTransferSetup APDU.");  
      }
      break;

    // CT-T4 Timeout
    case e_ctAwaitSetupResponse:
      OnReceivedSetupReturnError(H4502_CallTransferErrors::e_establishmentFailure, true);
      break;

    default:
      break;
  }
}


/////////////////////////////////////////////////////////////////////////////

H4504Handler::H4504Handler(H323Connection & conn, H450xDispatcher & disp)
  : H450xHandler(conn, disp)
{
  dispatcher.AddOpCode(H4504_CallHoldOperation::e_holdNotific, this);
  dispatcher.AddOpCode(H4504_CallHoldOperation::e_retrieveNotific, this);
  dispatcher.AddOpCode(H4504_CallHoldOperation::e_remoteHold, this);
  dispatcher.AddOpCode(H4504_CallHoldOperation::e_remoteRetrieve, this);

  holdState = e_ch_Idle;
}


PBoolean H4504Handler::OnReceivedInvoke(int opcode,
                                    int invokeId,
                                    int linkedId,
                                    PASN_OctetString *)
{
  currentInvokeId = invokeId;

  switch (opcode) {
    case H4504_CallHoldOperation::e_holdNotific:
      OnReceivedLocalCallHold(linkedId);
      break;

    case H4504_CallHoldOperation::e_retrieveNotific:
      OnReceivedLocalCallRetrieve(linkedId);
      break;

    case H4504_CallHoldOperation::e_remoteHold:
      OnReceivedRemoteCallHold(linkedId);
      break;

    case H4504_CallHoldOperation::e_remoteRetrieve:
      OnReceivedRemoteCallRetrieve(linkedId);
      break;

    default:
      currentInvokeId = 0;
      return PFalse;
  }

  return PTrue;
}


void H4504Handler::OnReceivedLocalCallHold(int /*linkedId*/)
{
  PTRACE(4, "H4504\tReceived a holdNotific Invoke APDU from the remote endpoint.");  
  // Optionally close our transmit channel.
}


void H4504Handler::OnReceivedLocalCallRetrieve(int /*linkedId*/)
{
  PTRACE(4, "H4504\tReceived a retrieveNotific Invoke APDU from the remote endpoint.");
  // Re-open our transmit channel if we previously closed it.
}


void H4504Handler::OnReceivedRemoteCallHold(int /*linkedId*/)
{
	// TBD
}


void H4504Handler::OnReceivedRemoteCallRetrieve(int /*linkedId*/)
{
	// TBD
}


bool H4504Handler::HoldCall(PBoolean localHold)
{
  // TBD: Implement Remote Hold. This implementation only does 
  // local hold. -- dcassel 4/01. 
  if (!localHold)
    return false;
  
  // Send a FACILITY message with a callNotific Invoke
  // Supplementary Service PDU to the held endpoint.
  PTRACE(4, "H4504\tTransmitting a holdNotific Invoke APDU to the remote endpoint.");

  H450ServiceAPDU serviceAPDU;

  currentInvokeId = dispatcher.GetNextInvokeId();
  serviceAPDU.BuildInvoke(currentInvokeId, H4504_CallHoldOperation::e_holdNotific);
  if (!serviceAPDU.WriteFacilityPDU(connection))
    return false;
  
  // Update hold state
  holdState = e_ch_NE_Held;
  return true;
}


bool H4504Handler::RetrieveCall()
{
  // TBD: Implement Remote Hold. This implementation only does

  // Send a FACILITY message with a retrieveNotific Invoke
  // Supplementary Service PDU to the held endpoint.
  PTRACE(4, "H4504\tTransmitting a retrieveNotific Invoke APDU to the remote endpoint.");

  H450ServiceAPDU serviceAPDU;

  currentInvokeId = dispatcher.GetNextInvokeId();
  serviceAPDU.BuildInvoke(currentInvokeId, H4504_CallHoldOperation::e_retrieveNotific);
  if (!serviceAPDU.WriteFacilityPDU(connection))
    return false;
  
  // Update hold state
  holdState = e_ch_Idle;
  return true;
}


/////////////////////////////////////////////////////////////////////////////

H4506Handler::H4506Handler(H323Connection & conn, H450xDispatcher & disp)
  : H450xHandler(conn, disp)
{
  dispatcher.AddOpCode(H4506_CallWaitingOperations::e_callWaiting, this);

  cwState = e_cw_Idle;
}


PBoolean H4506Handler::OnReceivedInvoke(int opcode,
                                    int invokeId,
                                    int linkedId,
                                    PASN_OctetString *argument)
{
  currentInvokeId = invokeId;

  switch (opcode) {
    case H4506_CallWaitingOperations::e_callWaiting:
      OnReceivedCallWaitingIndication(linkedId, argument);
      break;

    default:
      currentInvokeId = 0;
      return PFalse;
  }

  return PTrue;
}


void H4506Handler::OnReceivedCallWaitingIndication(int /*linkedId*/,
                                                   PASN_OctetString *argument)
{
  H4506_CallWaitingArg cwArg;

  if(!DecodeArguments(argument, cwArg, -1))
    return;

  connection.SetRemotCallWaiting(cwArg.m_nbOfAddWaitingCalls.GetValue());
  return;
}


void H4506Handler::AttachToAlerting(H323SignalPDU & pdu,
                                    unsigned numberOfCallsWaiting)
{
  PTRACE(4, "H450.6\tAttaching a Call Waiting Invoke PDU to this Alerting message.");
  
  H450ServiceAPDU serviceAPDU;

  // Store the call waiting invokeID associated with this connection
  currentInvokeId = dispatcher.GetNextInvokeId();

  serviceAPDU.BuildCallWaiting(currentInvokeId, numberOfCallsWaiting);
  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);

  cwState = e_cw_Invoked;
}
/**
 * Message waiting
 */

H4507Handler::H4507Handler(H323Connection & conn, H450xDispatcher & disp)
  : H450xHandler(conn, disp)
{
  mwiState = e_mwi_Idle;
  /* Allow to handle interrogate, only used by Mwi message center*/
  dispatcher.AddOpCode(H4507_H323_MWI_Operations::e_mwiInterrogate, this);
}


PBoolean H4507Handler::OnReceivedInvoke(int opcode,
                                    int invokeId,
                                    int linkedId,
                                    PASN_OctetString *argument)
{
  currentInvokeId = invokeId;
  PTRACE(3, "H450.7\tOnReceivedInvoke: invokeId = " << invokeId); 

  switch (opcode) {
    case H4507_H323_MWI_Operations::e_mwiActivate:
      endpoint.OnMWIReceived(connection.GetLocalPartyURL(), OpalManager::VoiceMessageWaiting, PString::Empty());
      break;

    case H4507_H323_MWI_Operations::e_mwiDeactivate:
      endpoint.OnMWIReceived(connection.GetLocalPartyURL(), OpalManager::NoMessageWaiting, PString::Empty());
      break;

    case H4507_H323_MWI_Operations::e_mwiInterrogate:
      break;

    default:
      PTRACE(2, "H450.7\tOnReceivedInvoke, not an interrogate");
      currentInvokeId = 0;
      return PFalse;
  }
  
  return PTrue;
}


void H4507Handler::AttachToSetup(H323SignalPDU & setupPDU,
                                 H4507_H323_MWI_Operations  &mwiOp, 
                                 PASN_Sequence &argument)
{
  PTRACE(3, "H450.7\tAttaching a Message waiting Invoke PDU to this setup message.");
  
  H450ServiceAPDU serviceAPDU;
  
  dispatcher.AddOpCode(mwiOp, this);
  
  // Store the message waiting invokeID associated with this connection
  currentInvokeId = dispatcher.GetNextInvokeId();

  serviceAPDU.BuildMessageWaiting(currentInvokeId, 
                                  mwiOp,
                                  argument);

  serviceAPDU.AttachSupplementaryServiceAPDU(setupPDU);

  mwiState = e_mwi_Invoked;
}

void H4507Handler::AttachInterrogateResultToPdu(H323SignalPDU & pdu,
                                                H4507_MWIInterrogateRes &interrogateResult)
{
  PTRACE(3, "H450.7\tAttachInterrogateResultToPdu");
  H450ServiceAPDU serviceAPDU;
  serviceAPDU.BuildInterrogateResult(currentInvokeId, interrogateResult);
  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);

  mwiState = e_mwi_Invoked;
}


void H4507Handler::AttachErrorToPdu(H323SignalPDU & pdu,
                                    H4507_MessageWaitingIndicationErrors error)
{
  PTRACE(3, "H450.7\tAttachErrorToPdu");
  H450ServiceAPDU serviceAPDU;
  serviceAPDU.BuildReturnError(currentInvokeId, error.GetValue());
  serviceAPDU.AttachSupplementaryServiceAPDU(pdu);

  mwiState = e_mwi_Invoked;
}



/////////////////////////////////////////////////////////////////////////////

H45011Handler::H45011Handler(H323Connection & conn, H450xDispatcher & disp)
  : H450xHandler(conn, disp)
{
  dispatcher.AddOpCode(H45011_H323CallIntrusionOperations::e_callIntrusionRequest, this);
  dispatcher.AddOpCode(H45011_H323CallIntrusionOperations::e_callIntrusionGetCIPL, this);
  dispatcher.AddOpCode(H45011_H323CallIntrusionOperations::e_callIntrusionIsolate, this);
  dispatcher.AddOpCode(H45011_H323CallIntrusionOperations::e_callIntrusionForcedRelease, this);
  dispatcher.AddOpCode(H45011_H323CallIntrusionOperations::e_callIntrusionWOBRequest, this);
  dispatcher.AddOpCode(H45011_H323CallIntrusionOperations::e_callIntrusionSilentMonitor, this);
  dispatcher.AddOpCode(H45011_H323CallIntrusionOperations::e_callIntrusionNotification, this);

  dispatcher.AddOpCode(H45010_H323CallOfferOperations::e_cfbOverride, this);
  dispatcher.AddOpCode(H45010_H323CallOfferOperations::e_remoteUserAlerting, this);

  dispatcher.AddOpCode(H4506_CallWaitingOperations::e_callWaiting, this);

  ciState = e_ci_Idle;
  ciSendState = e_ci_sIdle;
  ciReturnState = e_ci_rIdle;
  ciTimer.SetNotifier(PCREATE_NOTIFIER(OnCallIntrudeTimeOut));
}


PBoolean H45011Handler::OnReceivedInvoke(int opcode,
                                    int invokeId,
                                    int linkedId,
                                    PASN_OctetString * argument)
{
  PBoolean result = PTrue;
  currentInvokeId = invokeId;

  switch (opcode) {
    case H45011_H323CallIntrusionOperations::e_callIntrusionRequest:
      OnReceivedCallIntrusionRequest(linkedId, argument);
      break;

    case H45011_H323CallIntrusionOperations::e_callIntrusionGetCIPL:
      OnReceivedCallIntrusionGetCIPL(linkedId, argument);
      break;

    case H45011_H323CallIntrusionOperations::e_callIntrusionIsolate:
      OnReceivedCallIntrusionIsolate(linkedId, argument);
      break;

    case H45011_H323CallIntrusionOperations::e_callIntrusionForcedRelease:
      result = OnReceivedCallIntrusionForcedRelease(linkedId, argument);
      break;

    case H45011_H323CallIntrusionOperations::e_callIntrusionWOBRequest:
      OnReceivedCallIntrusionWOBRequest(linkedId, argument);
      break;

    case H45011_H323CallIntrusionOperations::e_callIntrusionSilentMonitor:
      OnReceivedCallIntrusionSilentMonitor(linkedId, argument);
      break;

    case H45011_H323CallIntrusionOperations::e_callIntrusionNotification:
      OnReceivedCallIntrusionNotification(linkedId, argument);
      break;

    case H45010_H323CallOfferOperations::e_cfbOverride:
      OnReceivedCfbOverride(linkedId, argument);
      break;

    case H45010_H323CallOfferOperations::e_remoteUserAlerting:
      OnReceivedRemoteUserAlerting(linkedId, argument);
      break;

    case H4506_CallWaitingOperations::e_callWaiting:
      OnReceivedCallWaiting(linkedId, argument);
      break;
    
    default:
      currentInvokeId = 0;
      return PFalse;
  }

  return result;
}


void H45011Handler::AttachToSetup(H323SignalPDU & pdu)
{
  // Do we need to attach a call transfer setup invoke APDU?
  if (ciSendState != e_ci_sAttachToSetup)
    return;

  H450ServiceAPDU serviceAPDU;

  // Store the outstanding invokeID associated with this connection
  currentInvokeId = dispatcher.GetNextInvokeId();
  PTRACE(4, "H450.11\tAttachToSetup Invoke ID=" << currentInvokeId);

  switch (ciGenerateState){
    case e_ci_gConferenceRequest:
      break;
    case e_ci_gHeldRequest:
      break;
    case e_ci_gSilentMonitorRequest:
      break;
    case e_ci_gIsolationRequest:
      break;
    case e_ci_gForcedReleaseRequest:
      serviceAPDU.BuildCallIntrusionForcedRelease(currentInvokeId, ciCICL);
      break;
    case e_ci_gWOBRequest:
      break;
    default:
      break;
  }
  
  if(ciGenerateState != e_ci_gIdle){
    // Use the call identity from the ctInitiateArg
    serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
    // start timer CT-T1
    PTRACE(4, "H450.11\tStarting timer CI-T1");
    StartciTimer(connection.GetEndPoint().GetCallIntrusionT1());
    ciState = e_ci_WaitAck;
  }
 
  ciSendState = e_ci_sIdle;
  ciGenerateState = e_ci_gIdle;
}


void H45011Handler::AttachToAlerting(H323SignalPDU & pdu)
{
  if (ciSendState != e_ci_sAttachToAlerting)
    return;

  PTRACE(4, "H450.11\tAttachToAlerting Invoke ID=" << currentInvokeId);

  // Store the outstanding invokeID associated with this connection
  currentInvokeId = dispatcher.GetNextInvokeId();
  PTRACE(4, "H450.11\tAttachToAlerting Invoke ID=" << currentInvokeId);
  if(ciReturnState!=e_ci_rIdle){
    H450ServiceAPDU serviceAPDU;
    switch (ciReturnState){
      case e_ci_rCallIntrusionImpending:
        serviceAPDU.BuildCallIntrusionImpending(currentInvokeId);
        PTRACE(4, "H450.11\tReturned e_ci_rCallIntrusionImpending");
        break;
      case e_ci_rCallIntruded:
        break;
      case e_ci_rCallIsolated:
        break;
      case e_ci_rCallForceReleased:
        break;
      case e_ci_rCallForceReleaseResult:
        serviceAPDU.BuildCallIntrusionForcedReleaseResult(currentInvokeId);
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionForced Release Result");
        break;
      case e_ci_rCallIntrusionComplete:
        break;
      case e_ci_rCallIntrusionEnd:
        break;
      case e_ci_rNotBusy:
        serviceAPDU.BuildReturnError(currentInvokeId, H45011_CallIntrusionErrors::e_notBusy);
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionErrors::e_notBusy");
        break;
      case e_ci_rTempUnavailable:
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionErrors::e_temporarilyUnavailable");
        serviceAPDU.BuildReturnError(currentInvokeId, H45011_CallIntrusionErrors::e_temporarilyUnavailable);
        break;
      case e_ci_rNotAuthorized:
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionErrors::e_notAuthorized");
        serviceAPDU.BuildReturnError(currentInvokeId, H45011_CallIntrusionErrors::e_notAuthorized);
        break;
      default :
        break;
    }
    serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
  }

  ciState = e_ci_Idle;
  ciSendState = e_ci_sIdle;
  ciReturnState = e_ci_rIdle;
}


void H45011Handler::AttachToConnect(H323SignalPDU & pdu)
{
  if ((currentInvokeId == 0) || (ciSendState != e_ci_sAttachToConnect))
    return;

  currentInvokeId = dispatcher.GetNextInvokeId();
  PTRACE(4, "H450.11\tAttachToConnect Invoke ID=" << currentInvokeId);
  if(ciReturnState!=e_ci_rIdle){
    H450ServiceAPDU serviceAPDU;
    switch (ciReturnState){
      case e_ci_rCallIntrusionImpending:
        break;
      case e_ci_rCallIntruded:
        break;
      case e_ci_rCallIsolated:
        break;
      case e_ci_rCallForceReleased:
        break;
      case e_ci_rCallForceReleaseResult:
        serviceAPDU.BuildCallIntrusionForcedReleaseResult(currentInvokeId);
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionForced Release Result");
        break;
      case e_ci_rCallIntrusionComplete:
        break;
      case e_ci_rCallIntrusionEnd:
        break;
      case e_ci_rNotBusy:
        serviceAPDU.BuildReturnError(currentInvokeId, H45011_CallIntrusionErrors::e_notBusy);
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionErrors::e_notBusy");
        break;
      case e_ci_rTempUnavailable:
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionErrors::e_temporarilyUnavailable");
        serviceAPDU.BuildReturnError(currentInvokeId, H45011_CallIntrusionErrors::e_temporarilyUnavailable);
        break;
      case e_ci_rNotAuthorized:
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionErrors::e_notAuthorized");
        serviceAPDU.BuildReturnError(currentInvokeId, H45011_CallIntrusionErrors::e_notAuthorized);
        break;
      default :
        break;
    }

    serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
  }


  ciState = e_ci_Idle;
  ciSendState = e_ci_sIdle;
  ciReturnState = e_ci_rIdle;
  currentInvokeId = 0;
}



void H45011Handler::AttachToReleaseComplete(H323SignalPDU & pdu)
{
  // Do we need to attach a call transfer setup invoke APDU?
  if (ciSendState != e_ci_sAttachToReleseComplete)
    return;

  PTRACE(4, "H450.11\tAttachToSetup Invoke ID=" << currentInvokeId);
  if(ciReturnState!=e_ci_rIdle){
    H450ServiceAPDU serviceAPDU;
    switch (ciReturnState){
      case e_ci_rNotBusy:
        serviceAPDU.BuildReturnError(currentInvokeId, H45011_CallIntrusionErrors::e_notBusy);
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionErrors::e_notBusy");
        break;
      case e_ci_rTempUnavailable:
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionErrors::e_temporarilyUnavailable");
        serviceAPDU.BuildReturnError(currentInvokeId, H45011_CallIntrusionErrors::e_temporarilyUnavailable);
        break;
      case e_ci_rNotAuthorized:
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionErrors::e_notAuthorized");
        serviceAPDU.BuildReturnError(currentInvokeId, H45011_CallIntrusionErrors::e_notAuthorized);
        break;
      case e_ci_rCallForceReleased:
        PTRACE(4, "H450.11\tReturned H45011_CallIntrusionForceRelease::e_ci_rCallForceReleased");
        serviceAPDU.BuildCallIntrusionForceRelesed(currentInvokeId);
        break;
      default :
        break;
    }
    serviceAPDU.AttachSupplementaryServiceAPDU(pdu);
  }

  ciState = e_ci_Idle;
  ciSendState = e_ci_sIdle;
  ciReturnState = e_ci_rIdle;
}


void H45011Handler::OnReceivedCallIntrusionRequest(int /*linkedId*/,
                                                   PASN_OctetString *argument)
{

  H45011_CIRequestArg ciArg;

  if(!DecodeArguments(argument, ciArg, -1))
    return;
/*
  TBD
*/
  return;
}


void H45011Handler::OnReceivedCallIntrusionGetCIPL(int /*linkedId*/,
                                                   PASN_OctetString *argument)
{
  PTRACE(4, "H450.11\tReceived GetCIPL Invoke");

  H45011_CIGetCIPLOptArg ciArg;

  // !!!!!!!!
  DecodeArguments(argument, ciArg, -1);
/*  if(!DecodeArguments(argument, ciArg, -1))
    return;
*/
  

  // Send a FACILITY message with a callTransferIdentify return result
  // Supplementary Service PDU to the transferring endpoint.
  H450ServiceAPDU serviceAPDU;

  X880_ReturnResult& result = serviceAPDU.BuildReturnResult(currentInvokeId);
  result.IncludeOptionalField(X880_ReturnResult::e_result);
  result.m_result.m_opcode.SetTag(X880_Code::e_local);
  PASN_Integer& operation = (PASN_Integer&) result.m_result.m_opcode;
  operation.SetValue(H45011_H323CallIntrusionOperations::e_callIntrusionGetCIPL);

  H45011_CIGetCIPLRes ciCIPLRes;

  ciCIPLRes.m_ciProtectionLevel = endpoint.GetCallIntrusionProtectionLevel();
  ciCIPLRes.IncludeOptionalField(H45011_CIGetCIPLRes::e_silentMonitoringPermitted);

  PPER_Stream resultStream;
  ciCIPLRes.Encode(resultStream);
  resultStream.CompleteEncoding();
  result.m_result.m_result.SetValue(resultStream);

  serviceAPDU.WriteFacilityPDU(connection);
  PTRACE(4, "H450.11\tSent GetCIPL Result CIPL=" << ciCIPLRes.m_ciProtectionLevel);
/*
  TBD
*/
  return;
}


void H45011Handler::OnReceivedCallIntrusionIsolate(int /*linkedId*/,
                                                   PASN_OctetString *argument)
{

  H45011_CIIsOptArg ciArg;

  if(!DecodeArguments(argument, ciArg, -1))
    return;
/*
  TBD
*/
  return;
}


PBoolean H45011Handler::OnReceivedCallIntrusionForcedRelease(int /*linkedId*/,
                                                         PASN_OctetString *argument)
{
  PBoolean result = PTrue;
  PTRACE(4, "H450.11\tReceived ForcedRelease Invoke");

  H45011_CIFrcRelArg ciArg;

  if(!DecodeArguments(argument, ciArg, -1))
    return PFalse;

  PStringList tokens = endpoint.GetAllConnections();

  if(tokens.GetSize() >1) {
    for (PStringList::iterator token = tokens.begin(); token != tokens.end(); ++token) {
      if(endpoint.HasConnection(*token)){
        PSafePtr<H323Connection> conn = endpoint.FindConnectionWithLock(*token, PSafeReadOnly);
        if (conn != NULL){
          if (conn->IsEstablished()){
            if((conn->GetLocalCallIntrusionProtectionLevel() < ciArg.m_ciCapabilityLevel)){
              activeCallToken = conn->GetCallToken();
              intrudingCallToken = connection.GetCallToken();
              conn->GetRemoteCallIntrusionProtectionLevel(connection.GetCallToken(), (unsigned)ciArg.m_ciCapabilityLevel);
              result = PTrue;
              break;
            }
            result = PFalse;
          }
        }
      }
    }

    if(result){
      ciSendState = e_ci_sAttachToConnect;
      ciReturnState = e_ci_rCallForceReleaseResult;
      connection.SetCallIntrusion ();
    }
    else {
      ciSendState = e_ci_sAttachToReleseComplete;
      ciReturnState = e_ci_rNotAuthorized;
      connection.ClearCall(H323Connection::EndedByLocalBusy);
    }
  }
  else {
    ciSendState = e_ci_sAttachToAlerting;
    ciReturnState = e_ci_rNotBusy;
  }

  return result;
}


void H45011Handler::OnReceivedCallIntrusionWOBRequest(int /*linkedId*/,
                                                      PASN_OctetString *argument)
{

  H45011_CIWobOptArg ciArg;

  if(!DecodeArguments(argument, ciArg, -1))
    return;
/*
  TBD
*/
  return;
}


void H45011Handler::OnReceivedCallIntrusionSilentMonitor(int /*linkedId*/,
                                                         PASN_OctetString *argument)
{

  H45011_CISilentArg ciArg;

  if(!DecodeArguments(argument, ciArg, -1))
    return;
/*
  TBD
*/
  return;
}


void H45011Handler::OnReceivedCallIntrusionNotification(int /*linkedId*/,
                                                        PASN_OctetString *argument)
{

  H45011_CINotificationArg ciArg;

  if(!DecodeArguments(argument, ciArg, -1))
    return;
/*
  TBD
*/
  return;
}


void H45011Handler::OnReceivedCfbOverride(int /*linkedId*/,
                                          PASN_OctetString *argument)
{

  H45010_CfbOvrOptArg ciArg;

  if(!DecodeArguments(argument, ciArg, -1))
    return;
/*
  TBD
*/
  return;
}


void H45011Handler::OnReceivedRemoteUserAlerting(int /*linkedId*/,
                                                 PASN_OctetString *argument)
{

  H45010_RUAlertOptArg ciArg;

  if(!DecodeArguments(argument, ciArg, -1))
    return;
/*
  TBD
*/
  return;
}


void H45011Handler::OnReceivedCallWaiting(int /*linkedId*/,
                                          PASN_OctetString *argument)
{

  H4506_CallWaitingArg ciArg;

  if(!DecodeArguments(argument, ciArg, -1))
    return;
/*
  TBD
*/
  return;
}


PBoolean H45011Handler::OnReceivedReturnResult(X880_ReturnResult & returnResult)
{
  PTRACE(4, "H450.11\tReceived Return Result");
  if (currentInvokeId == returnResult.m_invokeId.GetValue()) {
    PTRACE(4, "H450.11\tReceived Return Result Invoke ID=" << currentInvokeId);
    switch (ciState) {
      case e_ci_WaitAck:
        OnReceivedCIRequestResult(); 
        break;
      case e_ci_GetCIPL:
        OnReceivedCIGetCIPLResult(returnResult); 
        break;
      default :
        break;
    }
  }
  return PTrue;
}


void H45011Handler::OnReceivedCIRequestResult()
{
  PTRACE(4, "H450.11\tOnReceivedCIRequestResult");
  // stop timer CI-T1
  PTRACE(4, "H450.11\tTrying to stop timer CI-T1");
  StopciTimer();
}


void H45011Handler::OnReceivedCIGetCIPLResult(X880_ReturnResult & returnResult)
{
  PTRACE(4, "H450.11\tOnReceivedCIRequestResult");
  // Get the return result if present
  PASN_OctetString * result = NULL;
  if (returnResult.HasOptionalField(X880_ReturnResult::e_result)) {
    result = &returnResult.m_result.m_result;

    // Extract the C Party Details
    H45011_CIGetCIPLRes ciGetCIPLResult;

    PPER_Stream resultStream(*result);
    ciGetCIPLResult.Decode(resultStream);

    PTRACE(4 ,"H450.11\tReceived CIPL=" << ciGetCIPLResult.m_ciProtectionLevel );

    if (intrudingCallCICL > ciGetCIPLResult.m_ciProtectionLevel){

      // Send ciNotification.inv (ciImpending) To C
      H450ServiceAPDU serviceAPDU;
      currentInvokeId = dispatcher.GetNextInvokeId();
      serviceAPDU.BuildCallIntrusionImpending(currentInvokeId);
      serviceAPDU.WriteFacilityPDU(connection);

      // Send ciNotification.inv (ciImpending) to  intruding (A)
      PSafePtr<H323Connection> conn = endpoint.FindConnectionWithLock(intrudingCallToken);
      conn->SetIntrusionImpending();

      //Send Ringing to intruding (A)
      conn->AnsweringCall(conn->AnswerCallPending);

      // MUST RETURN ciNotification.inv (callForceRelesed) to active call (C) when releasing call !!!!!!
      ciSendState = e_ci_sAttachToReleseComplete;
      ciReturnState = e_ci_rCallForceReleased;
      
      //Send Forced Release Accepted when Answering call to intruding (A)
      conn->SetForcedReleaseAccepted();
    }
    else {
      PTRACE(4 ,"H450.11\tCICL<CIPL -> Clear Call");
      // Clear Call with intruding (A)
      PSafePtr<H323Connection> conn = endpoint.FindConnectionWithLock(intrudingCallToken);
      if (conn != NULL)
        conn->SetIntrusionNotAuthorized();
      endpoint.ClearCall(intrudingCallToken);
    }
  }

  // stop timer CI-T5
  PTRACE(4, "H450.11\tTrying to stop timer CI-T5");
  StopciTimer();
}


PBoolean H45011Handler::OnReceivedReturnError(int errorCode, X880_ReturnError &returnError)
{
  PBoolean result = PTrue;
  PTRACE(4, "H450.11\tReceived Return Error CODE=" <<errorCode << ", InvokeId=" <<returnError.m_invokeId.GetValue());
  if (currentInvokeId == returnError.m_invokeId.GetValue()) {
    switch (ciState) {
      case e_ci_WaitAck:
        result = OnReceivedInvokeReturnError(errorCode);
        break;
      case e_ci_GetCIPL:
        result = OnReceivedGetCIPLReturnError(errorCode);
        break;
      default :
        break;
    }
  }
  return result;
}


PBoolean H45011Handler::OnReceivedInvokeReturnError(int errorCode, const bool timerExpiry)
{
  PBoolean result = PFalse;
  PTRACE(4, "H450.11\tOnReceivedInvokeReturnError CODE =" << errorCode);
  if (!timerExpiry) {
    // stop timer CI-T1
    StopciTimer();
    PTRACE(4, "H450.11\tStopping timer CI-T1");
  }
  else
    PTRACE(4, "H450.11\tTimer CI-T1 has expired awaiting a response to a callIntrusionInvoke return result.");

  currentInvokeId = 0;
  ciState = e_ci_Idle;
  ciSendState = e_ci_sIdle;

  switch(errorCode){
    case H45011_CallIntrusionErrors::e_notBusy :
      PTRACE(4, "H450.11\tH45011_CallIntrusionErrors::e_notBusy");
      result = PTrue;
      break;
    case H45011_CallIntrusionErrors::e_temporarilyUnavailable :
      PTRACE(4, "H450.11\tH45011_CallIntrusionErrors::e_temporarilyUnavailable");
      break;
    case H45011_CallIntrusionErrors::e_notAuthorized :
      PTRACE(4, "H450.11\tH45011_CallIntrusionErrors::e_notAuthorized");
      result = PTrue;
      break;
    default:
      PTRACE(4, "H450.11\tH45011_CallIntrusionErrors::DEFAULT");
      break;
  }
  return result;
}


PBoolean H45011Handler::OnReceivedGetCIPLReturnError(int PTRACE_PARAM(errorCode),
                                                 const bool timerExpiry)
{
  PTRACE(4, "H450.11\tOnReceivedGetCIPLReturnError ErrorCode=" << errorCode);
  if(!timerExpiry){
    if (ciTimer.IsRunning()){
      ciTimer.Stop();
      PTRACE(4, "H450.11\tStopping timer CI-TX");
    }
  }

  // Send ciNotification.inv (ciImpending) to active call (C)
  H450ServiceAPDU serviceAPDU;
  currentInvokeId = dispatcher.GetNextInvokeId();
  serviceAPDU.BuildCallIntrusionImpending(currentInvokeId);
  serviceAPDU.WriteFacilityPDU(connection);

  // Send ciNotification.inv (ciImpending) to intruding (A)
  PSafePtr<H323Connection> conn = endpoint.FindConnectionWithLock(intrudingCallToken);
  if (conn != NULL) {
    conn->SetIntrusionImpending();

    //Send Ringing to intruding (A)
    conn->AnsweringCall(conn->AnswerCallPending);

    //Forced Release Accepted to send when Answering call to intruding (A)
    conn->SetForcedReleaseAccepted();
  }

  ciSendState = e_ci_sAttachToReleseComplete;
  ciReturnState = e_ci_rCallForceReleased;
      
  return PFalse;
}


void H45011Handler::IntrudeCall(int CICL)
{
  ciSendState = e_ci_sAttachToSetup;
  ciGenerateState = e_ci_gForcedReleaseRequest;
  ciCICL = CICL;
}


void H45011Handler::AwaitSetupResponse(const PString & token,
                                      const PString & identity)
{
  intrudingCallToken = token;
  intrudingCallIdentity = identity;
  ciState = e_ci_WaitAck;

}


PBoolean H45011Handler::GetRemoteCallIntrusionProtectionLevel(const PString & token,
                                                          unsigned intrusionCICL)
{
  intrudingCallToken = token;
  intrudingCallCICL = intrusionCICL;

  H450ServiceAPDU serviceAPDU;

  currentInvokeId = dispatcher.GetNextInvokeId();

  serviceAPDU.BuildCallIntrusionGetCIPL(currentInvokeId);

  if (!serviceAPDU.WriteFacilityPDU(connection))
    return PFalse;

  PTRACE(4, "H450.11\tStarting timer CI-T5");
  StartciTimer(connection.GetEndPoint().GetCallIntrusionT5());
  ciState = e_ci_GetCIPL;
  return PTrue;
}


void H45011Handler::SetIntrusionNotAuthorized()
{
  ciSendState = e_ci_sAttachToReleseComplete;
  ciReturnState = e_ci_rNotAuthorized;
}


void H45011Handler::SetIntrusionImpending()
{
  ciSendState = e_ci_sAttachToAlerting;
  ciReturnState = e_ci_rCallIntrusionImpending;
}


void H45011Handler::SetForcedReleaseAccepted()
{
  ciSendState = e_ci_sAttachToConnect;
  ciReturnState = e_ci_rCallForceReleaseResult;
  ciState = e_ci_DestNotify;

  StartciTimer(connection.GetEndPoint().GetCallIntrusionT6());
}


void H45011Handler::StopciTimer()
{
  if (ciTimer.IsRunning()){
    ciTimer.Stop();
    PTRACE(4, "H450.11\tStopping timer CI-TX");
  }
}


void H45011Handler::OnCallIntrudeTimeOut(PTimer &, INT)
{
  switch (ciState) {
    // CI-T1 Timeout
    case e_ci_WaitAck:
      PTRACE(4, "H450.11\tTimer CI-T1 has expired");
      OnReceivedInvokeReturnError(0,true);
      break;
    case e_ci_GetCIPL:
      PTRACE(4, "H450.11\tTimer CI-T5 has expired");
      OnReceivedGetCIPLReturnError(0,true);
      break;
    case e_ci_DestNotify:
      {
        PTRACE(4, "H450.11\tOnCallIntrudeTimeOut Timer CI-T6 has expired");
        // Clear the active call (call with C)
       PSyncPoint sync;
       endpoint.ClearCallSynchronous(activeCallToken, H323Connection::EndedByLocalUser, &sync);
        // Answer intruding call (call with A)
        PTRACE(4, "H450.11\tOnCallIntrudeTimeOut Trying to answer Call");
        if(endpoint.HasConnection(intrudingCallToken)){
          PSafePtr<H323Connection> conn = endpoint.FindConnectionWithLock(intrudingCallToken);
          if (conn != NULL)
            conn->AnsweringCall(H323Connection::AnswerCallNow);
        }
      }
      break;
    default:
      break;
  }
}


PBoolean H45011Handler::OnReceivedReject(int PTRACE_PARAM(problemType), int PTRACE_PARAM(problemNumber))
{
  PTRACE(4, "H450.11\tH45011Handler::OnReceivedReject - problemType= "
         << problemType << ", problemNumber= " << problemNumber);

  if (ciTimer.IsRunning()){
    ciTimer.Stop();
    PTRACE(4, "H450.11\tStopping timer CI-TX");
  }

  switch (ciState) {
    case e_ci_GetCIPL:
    {
      PSafePtr<H323Connection> conn = endpoint.FindConnectionWithLock(intrudingCallToken);
      conn->SetIntrusionImpending ();

      //Send Ringing to  intruding (A)
      conn->AnsweringCall (conn->AnswerCallPending);
      conn->SetForcedReleaseAccepted();  
      break;
    }

    default:
      break;
  }
  ciState = e_ci_Idle;
  return PTrue;
};

#endif // OPAL_H450
#endif // OPAL_H323

