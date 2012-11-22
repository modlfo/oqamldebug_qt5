open Printf
open Pervasives

let _ = 
  let s = 0 in
  let t = "Please\nEnter" in
  printf "Size? :" ;
  let m = read_int () in
  printf "%i\n" m
;;
