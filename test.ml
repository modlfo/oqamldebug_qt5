open Format
open Pervasives

let rec fact n = 
  match n with
    | 0 -> 1
    | n -> n * ( fact ( n - 1 ) ) 

let _ = 
  for n = 0 to 10 
  do
    printf "fact(%i) = %i\n" n (fact n)
  done
;;
