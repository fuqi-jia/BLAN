(set-info :smt-lib-version 2.6)
(set-logic QF_NIA)
(set-info :status sat)
(declare-fun x () Int)
(declare-fun y () Int)
(declare-fun z () Int)
(assert (= (- (* x y z) 4)  0))
(check-sat)
;(get-model)
(exit)