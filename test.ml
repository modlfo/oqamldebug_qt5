open Printf
open Pervasives

let _ = 
  let s = 0 in
  let t = [ 
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
    "Please\nEnter" ; "jnj" ; "" ; "hhhhhhhh" ; "nkjnkjn" ;
  ] in
    for i = 0 to 10
    do
  printf "." ;
    done ;
  printf "Size? :" ;
  let m = read_int () in
  printf "%i\n" m
;;
