open Format
open Pervasives

let rec fact n = 
  match n with
    | 0 -> 1
    | n -> n * ( fact ( n - 1 ) ) 

let _ = 
  let w = "Hello" in
  let w = "Hello all" in
  let w = "Hello world" in

  let n = 5 in
    printf "fact(%i) = %i\n" n (fact n)
;;
