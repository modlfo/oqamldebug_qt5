open Printf
open Pervasives

let _ = 
  let s = 0 in
  let t2 = [ "a" ; "b" ] in
  let t1 = [ "c" ; "d" ] in
  let t = [ t1 ; t2 ] in
    for i = 0 to 10
    do
      printf "." ;
    done ;
    printf "Size? :" ;
    let m = read_int () in
      printf "%i\n" m
;;
