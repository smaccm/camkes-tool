(*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 *)

/*- set thy = splitext(os.path.basename(options.outfile.name))[0] -*/
header {* Shared Memory *}
(*<*)
theory /*? thy ?*/ imports
  "../../tools/c-parser/CTranslation"
  "../../tools/autocorres/AutoCorres"
  "../../tools/autocorres/NonDetMonadEx"
begin

(* THIS THEORY IS GENERATED. DO NOT EDIT. *)

declare [[allow_underscore_idents=true]]

install_C_file "/*? thy ?*/.c"

autocorres [ts_rules = nondet] "/*? thy ?*/.c"

locale /*? thy ?*/ = /*? thy ?*/_glue
begin
(*>*)

lemma /*? me.from_interface.name ?*/__run_nf: "\<lbrace>\<lambda>s. \<forall>r. P r s\<rbrace> /*? me.from_interface.name ?*/__run' \<lbrace>P\<rbrace>!"
  apply (unfold /*? me.from_interface.name ?*/__run'_def)
  apply wp
  apply simp
  done

lemma /*? me.from_interface.name ?*/_wrap_ptr_nf:
  "\<lbrace>\<lambda>s. (\<forall>x\<in>set (array_addrs (Ptr (symbol_table ''/*? me.from_interface.name ?*/_data'')) /*? sizeof(me.from_interface.type) ?*/). is_valid_w8 s x) \<and>
        is_valid_dataport_ptr__C s x\<rbrace>
      /*? thy ?*/_wrap_ptr' x y
   \<lbrace>\<lambda>_ s. (\<forall>x\<in>set (array_addrs (Ptr (symbol_table ''/*? me.from_interface.name ?*/_data'')) /*? sizeof(me.from_interface.type) ?*/). is_valid_w8 s x) \<and>
                 is_valid_dataport_ptr__C s x\<rbrace>!"
  apply (unfold /*? me.from_interface.name ?*/_wrap_ptr'_def)
  apply wp
  apply simp
  done

/*# Leave this out for now.
(* Wrapping a valid dataport pointer returns success. XXX: You actually want to say more than this,
 * i.e. that the wrapper pointed is correct. Todo below.
 *)
lemma
  "\<lbrace>\<lambda>s. (\<forall>x\<in>set (array_addrs (Ptr (symbol_table ''/*? me.from_interface.name ?*/_data'')) /*? sizeof(me.from_interface.type) ?*/). is_valid_w8 s x) \<and>
        is_valid_dataport_ptr__C s x \<and>
        (ptr_val y) \<ge> (symbol_table ''/*? me.from_interface.name ?*/_data'') \<and>
        (ptr_val y) < (symbol_table ''/*? me.from_interface.name ?*/_data'') + /*? sizeof(me.from_interface.type) ?*/\<rbrace>
        /*? me.from_interface.name ?*/_wrap_ptr' x y
   \<lbrace>\<lambda>r s. r = 0 \<and>
          (\<forall>x\<in>set (array_addrs (Ptr (symbol_table ''/*? me.from_interface.name ?*/_data'')) /*? sizeof(me.from_interface.type) ?*/). is_valid_w8 s x) \<and>
          is_valid_dataport_ptr__C s x \<and>
          (ptr_val y) \<ge> (symbol_table ''/*? me.from_interface.name ?*/_data'') \<and>
          (ptr_val y) < (symbol_table ''/*? me.from_interface.name ?*/_data'') + /*? sizeof(me.from_interface.type) ?*/\<rbrace>!"
  apply (unfold /*? me.from_interface.name ?*/_wrap_ptr'_def)
  apply wp
  apply clarsimp
  apply unat_arith
  done
#*/

lemma /*? me.from_interface.name ?*/_unwrap_ptr_nf:
  "\<lbrace>\<lambda>s. (\<forall>x\<in>set (array_addrs (Ptr (symbol_table ''/*? me.from_interface.name ?*/_data'')) /*? sizeof(me.from_interface.type) ?*/). is_valid_w8 s x) \<and>
        is_valid_dataport_ptr__C s x\<rbrace>
     /*? me.from_interface.name ?*/_unwrap_ptr' x
   \<lbrace>\<lambda>_ s. (\<forall>x\<in>set (array_addrs (Ptr (symbol_table ''/*? me.from_interface.name ?*/_data'')) /*? sizeof(me.from_interface.type) ?*/). is_valid_w8 s x) \<and>
          is_valid_dataport_ptr__C s x\<rbrace>!"
  apply (unfold /*? me.from_interface.name ?*/_unwrap_ptr'_def)
  apply wp
  apply simp
  done

(*<*)
end

end
(*>*)
