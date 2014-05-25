// Copyright © 2011, Université catholique de Louvain
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// *  Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// *  Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef MOZART_BOOSTVM_H
#define MOZART_BOOSTVM_H

#include "boostvm-decl.hh"

#ifndef MOZART_GENERATOR

namespace mozart { namespace boostenv {

/////////////
// BoostVM //
/////////////

ProtectedNode BoostVM::allocAsyncIONode(StableNode* node) {
  _asyncIONodeCount++;
  return vm->protect(*node);
}

void BoostVM::releaseAsyncIONode(const ProtectedNode& node) {
  assert(_asyncIONodeCount > 0);
  _asyncIONodeCount--;
}

ProtectedNode BoostVM::createAsyncIOFeedbackNode(UnstableNode& readOnly) {
  StableNode* stable = new (vm) StableNode;
  stable->init(vm, Variable::build(vm));

  readOnly = ReadOnly::newReadOnly(vm, *stable);

  return allocAsyncIONode(stable);
}

template <class LT, class... Args>
void BoostVM::bindAndReleaseAsyncIOFeedbackNode(
  const ProtectedNode& ref, LT&& label, Args&&... args) {

  UnstableNode rhs = buildTuple(vm, std::forward<LT>(label),
                                std::forward<Args>(args)...);
  DataflowVariable(*ref).bind(vm, rhs);
  releaseAsyncIONode(ref);
}

template <class LT, class... Args>
void BoostVM::raiseAndReleaseAsyncIOFeedbackNode(
  const ProtectedNode& ref, LT&& label, Args&&... args) {

  UnstableNode exception = buildTuple(vm, std::forward<LT>(label),
                                      std::forward<Args>(args)...);
  bindAndReleaseAsyncIOFeedbackNode(
    ref, FailedValue::build(vm, RichNode(exception).getStableRef(vm)));
}

void BoostVM::postVMEvent(std::function<void()> callback) {
  {
    boost::unique_lock<boost::mutex> lock(_conditionWorkToDoInVMMutex);
    _vmEventsCallbacks.push(callback);
  }

  vm->requestExitRun();
  _conditionWorkToDoInVM.notify_all();
}

} }

#endif

#if !defined(MOZART_GENERATOR) && !defined(MOZART_BUILTIN_GENERATOR)
namespace mozart { namespace boostenv { namespace builtins {
#include "boostenvbuiltins.hh"
} } }
#endif

#endif // MOZART_BOOSTVM_H
