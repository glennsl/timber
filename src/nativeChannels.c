/**
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

 // This file has been adapted from reason-native/console:
 // https://github.com/facebookexperimental/reason-native/blob/master/src/console/nativeChannels.c

#define CAML_NAME_SPACE

#include <stdio.h>
#include <caml/mlvalues.h>
#include <caml/memory.h>

CAMLprim value timber_prerr_native(value str)
{
  CAMLparam1(str);
  fprintf(stderr, "%s", String_val(str));
  CAMLreturn(Val_unit);
}
