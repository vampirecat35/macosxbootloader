/*===---- ptrauth.h - Pointer authentication -------------------------------===
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 *===-----------------------------------------------------------------------===
 */

#ifndef __PTRAUTH_H
#define __PTRAUTH_H

typedef enum {
  ptrauth_key_asia = 0,
  ptrauth_key_asib = 1,
  ptrauth_key_asda = 2,
  ptrauth_key_asdb = 3,

  /* A process-independent key which can be used to sign code pointers.
     Signing and authenticating with this key is a no-op in processes
     which disable ABI pointer authentication. */
  ptrauth_key_process_independent_code = ptrauth_key_asia,

  /* A process-specific key which can be used to sign code pointers.
     Signing and authenticating with this key is enforced even in processes
     which disable ABI pointer authentication. */
  ptrauth_key_process_dependent_code = ptrauth_key_asib,

  /* A process-independent key which can be used to sign data pointers.
     Signing and authenticating with this key is a no-op in processes
     which disable ABI pointer authentication. */
  ptrauth_key_process_independent_data = ptrauth_key_asda,

  /* A process-specific key which can be used to sign data pointers.
     Signing and authenticating with this key is a no-op in processes
     which disable ABI pointer authentication. */
  ptrauth_key_process_dependent_data = ptrauth_key_asdb,

  /* The key used to sign C function pointers.
     The extra data is always 0. */
  ptrauth_key_function_pointer = ptrauth_key_process_independent_code,

  /* The key used to sign return addresses on the stack.
     The extra data is based on the storage address of the return address.
     On ARM64, that is always the storage address of the return address plus 8
     (or, in other words, the value of the stack pointer on function entry) */
  ptrauth_key_return_address = ptrauth_key_process_dependent_code,

  /* The key used to sign frame pointers on the stack.
     The extra data is based on the storage address of the frame pointer.
     On ARM64, that is always the storage address of the frame pointer plus 16
     (or, in other words, the value of the stack pointer on function entry) */
  ptrauth_key_frame_pointer = ptrauth_key_process_dependent_data,

  /* The key used to sign block function pointers, including:
       invocation functions,
       block object copy functions,
       block object destroy functions,
       __block variable copy functions, and
       __block variable destroy functions.
     The extra data is always the address at which the function pointer
     is stored.

     Note that block object pointers themselves (i.e. the direct
     representations of values of block-pointer type) are not signed. */
  ptrauth_key_block_function = ptrauth_key_asia,

  /* The key used to sign C++ v-table pointers.
     The extra data is always 0. */
  ptrauth_key_cxx_vtable_pointer = ptrauth_key_asda,

  /* The key used to sign metadata pointers to Objective-C method-lists. */
  ptrauth_key_method_list_pointer = ptrauth_key_asda,

  /* The key used to sign Objective-C isa and super pointers. */
  ptrauth_key_objc_isa_pointer = ptrauth_key_process_independent_data,
  ptrauth_key_objc_super_pointer = ptrauth_key_process_independent_data,

  /* The key used to sign block descriptor pointers. */
  ptrauth_key_block_descriptor_pointer = ptrauth_key_asda,

  /* Other pointers signed under the ABI use private ABI rules. */

} ptrauth_key;

/* An integer type of the appropriate size for a discriminator argument. */
typedef __UINTPTR_TYPE__ ptrauth_extra_data_t;

/* An integer type of the appropriate size for a generic signature. */
typedef __UINTPTR_TYPE__ ptrauth_generic_signature_t;

/* A signed pointer value embeds the original pointer together with
   a signature that attests to the validity of that pointer.  Because
   this signature must use only "spare" bits of the pointer, a
   signature's validity is probabilistic in practice: it is unlikely
   but still plausible that an invalidly-derived signature will
   somehow equal the correct signature and therefore successfully
   authenticate.  Nonetheless, this scheme provides a strong degree
   of protection against certain kinds of attacks. */

/* Authenticating a pointer that was not signed with the given key
   and extra-data value will (likely) fail by trapping. */

/* The null function pointer is always the all-zero bit pattern.
   Signing an all-zero bit pattern will embed a (likely) non-zero
   signature in the result, and so the result will not seem to be
   a null function pointer.  Authenticating this value will yield
   a null function pointer back.  However, authenticating an
   all-zero bit pattern will probably fail, because the
   authentication will expect a (likely) non-zero signature to
   embedded in the value.

   Because of this, if a pointer may validly be null, you should
   check for null before attempting to authenticate it with one
   of these intrinsics.  This is not necessary when using the
   __ptrauth qualifier; the compiler will perform this check
   automatically. */

#if __has_feature(ptrauth_qualifier_authentication_mode)
/* We define these macros including the comma so we can no-op them in the future
   without version locking with downstream projects
*/
  #if __has_feature(ptrauth_objc_isa_signs)
    #if __has_feature(ptrauth_objc_isa_authenticates)
      #define __ptrauth_isa_signing_mode , "sign-and-auth"
    #elif __has_feature(ptrauth_objc_isa_strips)
      #define __ptrauth_isa_signing_mode , "sign-and-strip"
    #endif
  #else
    #if __has_feature(ptrauth_objc_isa_strips)
      #define __ptrauth_isa_signing_mode , "strip"
    #else
      #define __ptrauth_isa_signing_mode
    #endif
  #endif
#else
  #define __ptrauth_isa_signing_mode
#endif

#ifdef __PTRAUTH_INTRINSICS__

/* Strip the signature from a value without authenticating it.

   If the value is a function pointer, the result will not be a
   legal function pointer because of the missing signature, and
   attempting to call it will result in an authentication failure.

   The value must be an expression of pointer type.
   The key must be a constant expression of type ptrauth_key.
   The result will have the same type as the original value. */
#define ptrauth_strip(__value, __key) \
  __builtin_ptrauth_strip(__value, __key)

/* Blend a constant discriminator into the given pointer-like value
   to form a new discriminator.  Not all bits of the inputs are
   guaranteed to contribute to the result.

   On arm64e, the integer must fall within the range of a uint16_t;
   other bits may be ignored.

   For the purposes of ptrauth_sign_constant, the result of calling
   this function is considered a constant expression if the arguments
   are constant.  Some restrictions may be imposed on the pointer.

   The first argument must be an expression of pointer type.
   The second argument must be an expression of integer type.
   The result will have type uintptr_t. */
#define ptrauth_blend_discriminator(__pointer, __integer) \
  __builtin_ptrauth_blend_discriminator(__pointer, __integer)

/* Add a signature to the given pointer value using a specific key,
   using the given extra data as a salt to the signing process.

   The value must be a constant expression of pointer type.
   The key must be a constant expression of type ptrauth_key.
   The extra data must be a constant expression of pointer or integer type;
   if an integer, it will be coerced to ptrauth_extra_data_t.
   The result will have the same type as the original value.

   This is a constant expression if the extra data is an integer or
   null pointer constant. */
#define ptrauth_sign_constant(__value, __key, __data) \
  __builtin_ptrauth_sign_constant(__value, __key, __data)

/* Add a signature to the given pointer value using a specific key,
   using the given extra data as a salt to the signing process.

   This operation does not authenticate the original value and is
   therefore potentially insecure if an attacker could possibly
   control that value.

   The value must be an expression of pointer type.
   The key must be a constant expression of type ptrauth_key.
   The extra data must be an expression of pointer or integer type;
   if an integer, it will be coerced to ptrauth_extra_data_t.
   The result will have the same type as the original value. */
#define ptrauth_sign_unauthenticated(__value, __key, __data) \
  __builtin_ptrauth_sign_unauthenticated(__value, __key, __data)

/* Authenticate a pointer using one scheme and resign it using another.

   If the result is subsequently authenticated using the new scheme, that
   authentication is guaranteed to fail if and only if the initial
   authentication failed.

   The value must be an expression of pointer type.
   The key must be a constant expression of type ptrauth_key.
   The extra data must be an expression of pointer or integer type;
   if an integer, it will be coerced to ptrauth_extra_data_t.
   The result will have the same type as the original value.

   This operation is guaranteed to not leave the intermediate value
   available for attack before it is re-signed.

   Do not pass a null pointer to this function. A null pointer
   will not successfully authenticate. */
#define ptrauth_auth_and_resign(__value, __old_key, __old_data, __new_key, __new_data) \
  __builtin_ptrauth_auth_and_resign(__value, __old_key, __old_data, __new_key, __new_data)

/* Authenticate a pointer using one scheme and resign it as a C
   function pointer.

   If the result is subsequently authenticated using the new scheme, that
   authentication is guaranteed to fail if and only if the initial
   authentication failed.

   The value must be an expression of function pointer type.
   The key must be a constant expression of type ptrauth_key.
   The extra data must be an expression of pointer or integer type;
   if an integer, it will be coerced to ptrauth_extra_data_t.
   The result will have the same type as the original value.

   This operation is guaranteed to not leave the intermediate value
   available for attack before it is re-signed. Additionally, if this
   expression is used syntactically as the function expression in a
   call, only a single authentication will be performed. */
#define ptrauth_auth_function(__value, __old_key, __old_data) \
  ptrauth_auth_and_resign(__value, __old_key, __old_data, ptrauth_key_function_pointer, 0)

/* Cast a pointer to the given type without changing any signature.

   The type must be a pointer type.
   The value must be an expression of function pointer type, and will be
   converted to an rvalue prior to the cast.
   The result has type given by the first argument.

   The result has an identical bit-pattern to the input pointer. */
#define ptrauth_nop_cast(__type, __value)        \
  ({ union {                                     \
      typeof(*(__value)) *__fptr;                \
      typeof(__type) __opaque;                   \
  } __storage;                                   \
  __storage.__fptr = (__value);                  \
  __storage.__opaque; })

/* Authenticate a data pointer.

   The value must be an expression of non-function pointer type.
   The key must be a constant expression of type ptrauth_key.
   The extra data must be an expression of pointer or integer type;
   if an integer, it will be coerced to ptrauth_extra_data_t.
   The result will have the same type as the original value.

   If the authentication fails, dereferencing the resulting pointer
   will fail. */
#define ptrauth_auth_data(__value, __old_key, __old_data) \
  __builtin_ptrauth_auth(__value, __old_key, __old_data)

/* Compute a constant discriminator from the given string.

   The result can be used as the second argument to
   ptrauth_blend_discriminator or the third argument to the
   __ptrauth qualifier.  It has type size_t.

   The argument must be a string literal.
   A call to this function is an integer constant expression. */
#define ptrauth_string_discriminator(__string) \
  __builtin_ptrauth_string_discriminator(__string)

/* Compute a constant discriminator from the given type.

   The result can be used as the second argument to
   ptrauth_blend_discriminator or the third argument to the
   __ptrauth qualifier.  It has type size_t.

   If the type is a C++ member function pointer type, the result is
   the discriminator used to signed member function pointers of that
   type.  If the type is a function, function pointer, or function
   reference type, the result is the discriminator used to sign
   functions of that type.  It is ill-formed to use this macro with any
   other type.

   A call to this function is an integer constant expression. */
#define ptrauth_type_discriminator(__type) \
  __builtin_ptrauth_type_discriminator(__type)

/* Compute the constant discriminator used by Clang to sign pointers with the
   given C function pointer type.

   A call to this function is an integer constant expression*/
#if __has_feature(ptrauth_function_pointer_type_discrimination)
#define ptrauth_function_pointer_type_discriminator(__type) \
  __builtin_ptrauth_type_discriminator(__type)
#else
#define ptrauth_function_pointer_type_discriminator(__type) ((ptrauth_extra_data_t)0)
#endif



/* Compute a signature for the given pair of pointer-sized values.
   The order of the arguments is significant.

   Like a pointer signature, the resulting signature depends on
   private key data and therefore should not be reliably reproducible
   by attackers.  That means that this can be used to validate the
   integrity of arbitrary data by storing a signature for that data
   alongside it, then checking that the signature is still valid later.
   Data which exceeds two pointers in size can be signed by either
   computing a tree of generic signatures or just signing an ordinary
   cryptographic hash of the data.

   The result has type ptrauth_generic_signature_t.  However, it may
   not have as many bits of entropy as that type's width would suggest;
   some implementations are known to compute a compressed signature as
   if the arguments were a pointer and a discriminator.

   The arguments must be either pointers or integers; if integers, they
   will be coerce to uintptr_t. */
#define ptrauth_sign_generic_data(__value, __data) \
  __builtin_ptrauth_sign_generic_data(__value, __data)

/* Define some standard __ptrauth qualifiers used in the ABI. */
#define __ptrauth_function_pointer(__typekey) \
  __ptrauth(ptrauth_key_function_pointer,0,__typekey)
#define __ptrauth_return_address              \
  __ptrauth(ptrauth_key_return_address,1,0)
#define __ptrauth_block_invocation_pointer    \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_block_copy_helper           \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_block_destroy_helper        \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_block_byref_copy_helper     \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#define __ptrauth_block_byref_destroy_helper  \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#if __has_feature(ptrauth_signed_block_descriptors)
#define __ptrauth_block_descriptor_pointer \
  __ptrauth(ptrauth_key_block_descriptor_pointer,1,0xC0BB)
#else
#define __ptrauth_block_descriptor_pointer
#endif
#define __ptrauth_objc_method_list_imp        \
  __ptrauth(ptrauth_key_function_pointer,1,0)
#if __has_feature(ptrauth_objc_method_list_pointer)
#define __ptrauth_objc_method_list_pointer \
  __ptrauth(ptrauth_key_method_list_pointer,1,0xC310)
#else
#define __ptrauth_objc_method_list_pointer
#endif
#if !__has_feature(ptrauth_qualifier_authentication_mode) ||                   \
      (__has_feature(ptrauth_objc_isa_signs) ||                                \
       __has_feature(ptrauth_objc_isa_strips))
#define __ptrauth_isa_discriminator 0x6AE1
#define __ptrauth_super_discriminator 0xB5AB
#define __ptrauth_objc_isa_pointer                                             \
  __ptrauth(ptrauth_key_objc_isa_pointer, 1,                                   \
            __ptrauth_isa_discriminator __ptrauth_isa_signing_mode)
#define __ptrauth_objc_isa_uintptr                                             \
  __ptrauth_restricted_intptr(                                                 \
      ptrauth_key_objc_isa_pointer, 1,                                         \
      __ptrauth_isa_discriminator __ptrauth_isa_signing_mode)
#define __ptrauth_objc_super_pointer                                           \
  __ptrauth(ptrauth_key_objc_super_pointer, 1,                                 \
            __ptrauth_super_discriminator __ptrauth_isa_signing_mode)
#else
#define __ptrauth_objc_isa_pointer
#define __ptrauth_objc_isa_uintptr
#define __ptrauth_objc_super_pointer
#endif
#define __ptrauth_cxx_vtable_pointer          \
  __ptrauth(ptrauth_key_cxx_vtable_pointer,0,0)
#define __ptrauth_cxx_vtt_vtable_pointer      \
  __ptrauth(ptrauth_key_cxx_vtable_pointer,0,0)
#define __ptrauth_swift_heap_object_destructor \
  __ptrauth(ptrauth_key_function_pointer,1,0xbbbf)

/* Some situations in the C++ and Swift ABIs use declaration-specific
   or type-specific extra discriminators. */
#define __ptrauth_cxx_virtual_function_pointer(__declkey) \
  __ptrauth(ptrauth_key_function_pointer,1,__declkey)
#define __ptrauth_swift_function_pointer(__typekey) \
  __ptrauth(ptrauth_key_function_pointer,0,__typekey)
#define __ptrauth_swift_class_method_pointer(__declkey) \
  __ptrauth(ptrauth_key_function_pointer,1,__declkey)
#define __ptrauth_swift_protocol_witness_function_pointer(__declkey) \
  __ptrauth(ptrauth_key_function_pointer,1,__declkey)
#define __ptrauth_swift_value_witness_function_pointer(__key) \
  __ptrauth(ptrauth_key_function_pointer,1,__key)

/* C++ vtable pointer signing class attribute */
#define ptrauth_cxx_vtable_pointer(key, address_discrimination,                \
                                   extra_discrimination...)                    \
  [[clang::ptrauth_vtable_pointer(key, address_discrimination,                 \
                                  extra_discrimination)]]

#else

#define ptrauth_strip(__value, __key) __value
#define ptrauth_blend_discriminator(__pointer, __integer) ((ptrauth_extra_data_t)0)
#define ptrauth_sign_constant(__value, __key, __data) __value
#define ptrauth_sign_unauthenticated(__value, __key, __data) __value
#define ptrauth_auth_and_resign(__value, __old_key, __old_data, __new_key, __new_data) __value
#define ptrauth_auth_function(__value, __old_key, __old_data) __value
#define ptrauth_nop_cast(__type, __value) (__type)(__value)
#define ptrauth_auth_data(__value, __old_key, __old_data) __value
#define ptrauth_string_discriminator(__string) ((ptrauth_extra_data_t)0)
#define ptrauth_type_discriminator(__type) ((ptrauth_extra_data_t)0)
#define ptrauth_function_pointer_type_discriminator(__type) ((ptrauth_extra_data_t)0)
#define ptrauth_sign_generic_data(__value, __data) ((ptrauth_generic_signature_t)0)

#define __ptrauth_function_pointer(__typekey)
#define __ptrauth_return_address
#define __ptrauth_block_invocation_pointer
#define __ptrauth_block_copy_helper
#define __ptrauth_block_destroy_helper
#define __ptrauth_block_byref_copy_helper
#define __ptrauth_block_byref_destroy_helper
#define __ptrauth_block_descriptor_pointer
#define __ptrauth_objc_method_list_imp
#define __ptrauth_objc_method_list_pointer
#define __ptrauth_objc_isa_pointer
#define __ptrauth_objc_isa_uintptr
#define __ptrauth_objc_super_pointer
#define __ptrauth_cxx_vtable_pointer
#define __ptrauth_cxx_vtt_vtable_pointer
#define __ptrauth_swift_heap_object_destructor
#define __ptrauth_cxx_virtual_function_pointer(__declkey)
#define __ptrauth_swift_function_pointer(__typekey)
#define __ptrauth_swift_class_method_pointer(__declkey)
#define __ptrauth_swift_protocol_witness_function_pointer(__declkey)
#define __ptrauth_swift_value_witness_function_pointer(__key)
#define ptrauth_cxx_vtable_pointer(key, address_discrimination,                \
                                   extra_discrimination...)

#endif /* __PTRAUTH_INTRINSICS__ */

#endif /* __PTRAUTH_H */
