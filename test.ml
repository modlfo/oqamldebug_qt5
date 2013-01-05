open Format
open Pervasives

let rec fact n = 
  match n with
    | 0 -> 1
    | n -> n * ( fact ( n - 1 ) ) 

let _ = 
  for i = 0 to 10000000 do fact 5 ; done ;
  let n = 5 in
    printf "fact(%i) = %i\n" n (fact n)
;;
