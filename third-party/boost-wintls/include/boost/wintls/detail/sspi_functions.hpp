//
// Copyright (c) 2020 Kasper Laudrup (laudrup at stacktrace dot dk)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_WINTLS_DETAIL_SSPI_FUNCTIONS_HPP
#define BOOST_WINTLS_DETAIL_SSPI_FUNCTIONS_HPP

#include <boost/wintls/detail/sspi_types.hpp>

namespace boost {
namespace wintls {
namespace detail {
namespace sspi_functions {

inline SecurityFunctionTable* sspi_function_table() {
  static SecurityFunctionTable* impl = InitSecurityInterface();
  // TODO: Figure out some way to signal this to the user instead of aborting
  BOOST_ASSERT_MSG(impl != nullptr, "Unable to initialize SecurityFunctionTable");
  return impl;
}

inline SECURITY_STATUS AcquireCredentialsHandle(SEC_CHAR* pPrincipal,
                                                SEC_CHAR* pPackage,
                                                unsigned long fCredentialUse,
                                                void* pvLogonId,
                                                void* pAuthData,
                                                SEC_GET_KEY_FN pGetKeyFn,
                                                void* pvGetKeyArgument,
                                                PCredHandle phCredential,
                                                PTimeStamp ptsExpiry) {
  return sspi_function_table()->AcquireCredentialsHandle(pPrincipal,
                                                          pPackage,
                                                          fCredentialUse,
                                                          pvLogonId,
                                                          pAuthData,
                                                          pGetKeyFn,
                                                          pvGetKeyArgument,
                                                          phCredential,
                                                          ptsExpiry);
}

inline SECURITY_STATUS DeleteSecurityContext(PCtxtHandle phContext) {
  return sspi_function_table()->DeleteSecurityContext(phContext);
}

inline SECURITY_STATUS InitializeSecurityContext(PCredHandle phCredential,
                                                 PCtxtHandle phContext,
                                                 SEC_CHAR* pTargetName,
                                                 unsigned long fContextReq,
                                                 unsigned long Reserved1,
                                                 unsigned long TargetDataRep,
                                                 PSecBufferDesc pInput,
                                                 unsigned long Reserved2,
                                                 PCtxtHandle phNewContext,
                                                 PSecBufferDesc pOutput,
                                                 unsigned long* pfContextAttr,
                                                 PTimeStamp ptsExpiry) {
  return sspi_function_table()->InitializeSecurityContext(phCredential,
                                                           phContext,
                                                           pTargetName,
                                                           fContextReq,
                                                           Reserved1,
                                                           TargetDataRep,
                                                           pInput,
                                                           Reserved2,
                                                           phNewContext,
                                                           pOutput,
                                                           pfContextAttr,
                                                           ptsExpiry);
}

inline SECURITY_STATUS FreeContextBuffer(PVOID pvContextBuffer) {
  return sspi_function_table()->FreeContextBuffer(pvContextBuffer);
}

inline SECURITY_STATUS DecryptMessage(PCtxtHandle phContext, PSecBufferDesc pMessage, unsigned long MessageSeqNo, unsigned long* pfQOP) {
  return sspi_function_table()->DecryptMessage(phContext, pMessage, MessageSeqNo, pfQOP);
}

inline SECURITY_STATUS QueryContextAttributes(PCtxtHandle phContext, unsigned long ulAttribute, void* pBuffer) {
  return sspi_function_table()->QueryContextAttributes(phContext, ulAttribute, pBuffer);
}

inline SECURITY_STATUS EncryptMessage(PCtxtHandle phContext, unsigned long fQOP, PSecBufferDesc pMessage, unsigned long MessageSeqNo) {
  return sspi_function_table()->EncryptMessage(phContext, fQOP, pMessage, MessageSeqNo);
}

inline SECURITY_STATUS FreeCredentialsHandle(PCredHandle phCredential) {
  return sspi_function_table()->FreeCredentialsHandle(phCredential);
}

inline SECURITY_STATUS ApplyControlToken(PCtxtHandle phContext, PSecBufferDesc pInput) {
  return sspi_function_table()->ApplyControlToken(phContext, pInput);
}

inline SECURITY_STATUS AcceptSecurityContext(PCredHandle phCredential,
                                             PCtxtHandle phContext,
                                             PSecBufferDesc pInput,
                                             unsigned long fContextReq,
                                             unsigned long TargetDataRep,
                                             PCtxtHandle phNewContext,
                                             PSecBufferDesc pOutput,
                                             unsigned long* pfContextAttr,
                                             PTimeStamp ptsExpiry) {
  return sspi_function_table()->AcceptSecurityContext(phCredential,
                                                      phContext,
                                                      pInput,
                                                      fContextReq,
                                                      TargetDataRep,
                                                      phNewContext,
                                                      pOutput,
                                                      pfContextAttr,
                                                      ptsExpiry);
}
} // namespace sspi_functions
} // namespace detail
} // namespace wintls
} // namespace boost

#endif // BOOST_WINTLS_DETAIL_SSPI_FUNCTIONS_HPP
