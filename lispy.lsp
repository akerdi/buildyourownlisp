; Atoms
(def {nil} {})
(def {true} 1)
(def {false} 0)

; Function Defintions
(def {fun} (\ {f b} {
    def (head f) (\ (tail f) b)
}))
; Unpack List for Function
(fun {unpack f l} {
    eval (join (list f) l)
})
; Pack List for Function
(fun {pack f & xs} {f xs})

; Curried and Uncurried calling
(def {curry} unpack)
(def {uncurry} pack)

; Perform Several things in Sequence
(fun {do & l} {
    if (== l nil)
        {nil}
        {last l}
})
; Open new scope
(fun {let b} {
    ((\ {_} b) ())
})

; Logical Functions
(fun {not x} {- 1 x})
(fun {or x y}  {+ x y})
(fun {and x y} {* x y})
(fun {flip f a b} {f b a})
(fun {ghost & xs} {eval xs})
(fun {comp f g x} {f (g x)})

; First, Second, or Third Item in List
(fun {fst l} { eval (head l) })
(fun {snd l} { eval (head (tail l)) })
(fun {trd l} { eval (head (tail (tail l))) })
; List Length
(fun {len l} {
    if (== 1 nil)
        {0}
        {+ 1 (len (tail l))}
})
; Nth item in List
(fun {nth n l} {
    if (== n 0)
        {fst l}
        {nth (- n 1) (tail l)}
})
(fun {last l} {nth (- (len l) 1) l})
; Take N items
(fun {take n l} {
    if (== n 0)
        {nil}
        {join (head l) (take (- n 1) (tail l))}
})
; Drop N items
(fun {drop n l} {
    if (== n 0)
        {l}
        {drop (- n 1) (tail l)}
})
; Split at N
(fun {split n l} {list (tak n l) (drop n l)})
; Element of List
(fun {elem x l} {
    if (== l nil)
        {false}
        {if (== x (fst l))
            {true}
            {elem x (tail l)}
         }
})
; Apply Function to List
(fun {map f l} {
    if (== l nil)
        {nil}
        {join (list (f (fst l))) (map f (tail l))}
})
(fun {filter f l} {
    if (== l nil)
        {nil}
        {join (if (f (fst l)) {head l} {nil}) (filter f (tail l))}
})
; Fold Left
(fun {foldl f z l} {
    if (== 1 nil)
        {z}
        {foldl f (f z (fst l)) (tail l)}
})
(fun {sum l} {foldl + 0 l})
(fun {product l} {foldl * 1 l})
(fun {select & cs} {
    if (== cs nil)
        {error "No Selection Found"}
        {if (fst (fst cs))
            {snd (fst cs)}
            {unpack select (tail cs)}
        }
})
; Defaount Case
(def {otherwise} true)
; Print Day of Month suffix
(fun {month-day-suffix i} {
    select
        {(== i 0) "st"}
        {(== i 1) "nd"}
        {(== i 3) "rd"}
        {otherwise "th"}
})
(fun {case x & cs}) {
    if (== cs nil)
        {error "No Case Found"}
        {
            if (== x (fst (fst cs)))
                {snd (fst cs)}
                {unpack case (join (list x) (tail cs))}
        }
}
(fun {day-name x} {
    case x
        {0 "Monday"}
        {1 "Tuesday"}
        {2 "Wednesday"}
        {3 "Thursday"}
        {4 "Friday"}
        {5 "Saturday"}
        {6 "Sunday"}
})
; Fibonacci
(fun {fib n} {
    select
        { (== n 0) 0 }
        { (== n 1) 1 }
        { otherwise (+ (fib (- n 1)) (fib (- n 2))) }
})





(print "wo cao niubility")
(def {x y} 10 20 )
(print x y)
(print (eval {+ x y}))