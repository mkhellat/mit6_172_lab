;;; Constraint Solver for Parallel Execution Time Measurements
;;; 
;;; This LISP program checks consistency of parallel execution time measurements
;;; using the Work Law, Span Law, and Greedy Scheduler Bound.
;;;
;;; Usage: (check-measurements '((4 80) (10 42) (64 9)))
;;;        Returns: (inconsistent "330 <= T1 <= 320 is impossible")

(defpackage :constraint-solver
  (:use :common-lisp)
  (:export :check-measurements
           :derive-constraints
           :check-pairwise-compatibility
           :find-contradiction
           :format-result-readable
           :format-result-json))

(in-package :constraint-solver)

;;; ============================================================================
;;; Constraint Derivation
;;; ============================================================================

(defun derive-work-law (p tp)
  "Derive Work Law constraint: T1 <= P * TP"
  (* p tp))

(defun derive-span-law (tp)
  "Derive Span Law constraint: T_infinity <= TP"
  tp)

(defun derive-greedy-bound (p tp)
  "Derive Greedy Scheduler Bound: T1 >= P * TP - (P-1) * T_infinity
   Returns a function: (lambda (t-inf) (- (* p tp) (* (- p 1) t-inf)))"
  (lambda (t-inf)
    (- (* p tp) (* (- p 1) t-inf))))

(defun derive-constraints (measurements)
  "Derive all constraints from measurements.
   Returns: ((work-law . value) (span-law . value) (greedy-bound . function))"
  (mapcar (lambda (m)
            (let ((p (first m))
                  (tp (second m)))
              (list :processor-count p
                    :time tp
                    :work-law (derive-work-law p tp)
                    :span-law (derive-span-law tp)
                    :greedy-bound (derive-greedy-bound p tp))))
          measurements))

;;; ============================================================================
;;; Constraint Checking
;;; ============================================================================

(defun find-tightest-span-constraint (constraints)
  "Find the tightest (minimum) span constraint from all measurements"
  (reduce #'min (mapcar (lambda (c) (getf c :span-law)) constraints)))

(defun evaluate-greedy-bounds (constraints t-infinity)
  "Evaluate greedy scheduler bounds at a given T_infinity value"
  (mapcar (lambda (c)
            (list :processor-count (getf c :processor-count)
                  :time (getf c :time)
                  :greedy-bound-value (funcall (getf c :greedy-bound) t-infinity)
                  :work-law (getf c :work-law)))
          constraints))

(defun find-feasible-region (constraints t-infinity)
  "Find the feasible region for T1 given T_infinity"
  (let ((greedy-bounds (evaluate-greedy-bounds constraints t-infinity)))
    (let ((min-t1 (reduce #'max (mapcar (lambda (c) (getf c :greedy-bound-value)) greedy-bounds)))
          (max-t1 (reduce #'min (mapcar (lambda (c) (getf c :work-law)) greedy-bounds))))
      (list :min-t1 min-t1
            :max-t1 max-t1
            :feasible (<= min-t1 max-t1)
            :t-infinity t-infinity))))

(defun find-contradiction (measurements)
  "Check if measurements are consistent. Returns contradiction info if inconsistent."
  (let* ((constraints (derive-constraints measurements))
         (t-infinity-max (find-tightest-span-constraint constraints))
         (feasible-region (find-feasible-region constraints t-infinity-max)))
    (if (getf feasible-region :feasible)
        (list :status :consistent
              :feasible-region feasible-region)
        (list :status :inconsistent
              :contradiction (format nil "~A <= T1 <= ~A is impossible"
                                    (getf feasible-region :min-t1)
                                    (getf feasible-region :max-t1))
              :min-t1 (getf feasible-region :min-t1)
              :max-t1 (getf feasible-region :max-t1)
              :t-infinity-max t-infinity-max
              :feasible-region feasible-region))))

;;; ============================================================================
;;; Pairwise Compatibility Checking
;;; ============================================================================

(defun check-pairwise-compatibility (measurements)
  "Check compatibility of all pairs of measurements"
  (let ((pairs '())
        (n (length measurements)))
    (loop for i from 0 below n do
      (loop for j from (+ i 1) below n do
        (let* ((pair (list (nth i measurements) (nth j measurements)))
               (constraints (derive-constraints pair))
               (t-infinity-max (find-tightest-span-constraint constraints))
               (feasible-region (find-feasible-region constraints t-infinity-max)))
          (push (list :pair pair
                      :compatible (getf feasible-region :feasible)
                      :feasible-region feasible-region)
                pairs))))
    (reverse pairs)))

;;; ============================================================================
;;; Main Interface
;;; ============================================================================

(defun check-measurements (measurements)
  "Main function to check measurement consistency.
   Measurements format: ((P1 T1) (P2 T2) ...)
   Example: (check-measurements '((4 80) (10 42) (64 9)))"
  (let* ((result (find-contradiction measurements))
         (pairwise (check-pairwise-compatibility measurements)))
    (list :overall result
          :pairwise pairwise
          :measurements measurements)))

;;; ============================================================================
;;; Output Formatting (for JSON/CLI output)
;;; ============================================================================

(defun format-result-json (result)
  "Format result as JSON string for Python consumption"
  (let ((overall (getf result :overall))
        (pairwise (getf result :pairwise)))
    (let ((contradiction-str (if (getf overall :contradiction)
                                 (format nil "\"~A\"" (getf overall :contradiction))
                                 "null"))
          (pairwise-json (mapcar (lambda (p)
                                   (let ((pair (getf p :pair))
                                         (compatible (getf p :compatible)))
                                     (format nil "~%    {\"pair\": [[~A, ~A], [~A, ~A]], \"compatible\": ~A}"
                                             (first (first pair))
                                             (second (first pair))
                                             (first (second pair))
                                             (second (second pair))
                                             (if compatible "true" "false"))))
                                 pairwise)))
      (format nil "{~%  \"status\": \"~A\",~%  \"contradiction\": ~A,~%  \"pairwise\": [~{~A~^,~}~%  ]~%}"
              (string-downcase (string (getf overall :status)))
              contradiction-str
              pairwise-json))))

(defun format-result-readable (result)
  "Format result in human-readable format"
  (let ((overall (getf result :overall))
        (pairwise (getf result :pairwise)))
    (format t "~%=== Measurement Consistency Check ===~%")
    (format t "Overall Status: ~A~%" (getf overall :status))
    (when (eq (getf overall :status) :inconsistent)
      (format t "Contradiction: ~A~%" (getf overall :contradiction))
      (let ((fr (getf overall :feasible-region)))
        (format t "  T1 range: ~A <= T1 <= ~A~%" 
                (getf fr :min-t1) (getf fr :max-t1))
        (format t "  T_infinity <= ~A~%" (getf overall :t-infinity-max))))
    (format t "~%Pairwise Compatibility:~%")
    (dolist (p pairwise)
      (format t "  ~A: ~A~%" 
              (getf p :pair)
              (if (getf p :compatible) "COMPATIBLE" "INCOMPATIBLE")))))

;;; ============================================================================
;;; CLI Interface
;;; ============================================================================

(defun main ()
  "CLI entry point for the constraint solver"
  (let ((args (cdr *posix-argv*)))
    (if (null args)
        (format t "Usage: constraint-solver.lisp '((4 80) (10 42) (64 9))'~%")
        (let* ((measurements (read-from-string (first args)))
               (result (check-measurements measurements)))
          (format-result-readable result)
          (format-result-json result)))))

;;; Run main if executed directly
;;; Note: This works with SBCL. For ECL, use the Python interface.
#+sbcl
(when (and (boundp '*load-truename*)
           (member (pathname-name *load-truename*) 
                   (list "constraint-solver" "constraint-solver.lisp")
                   :test #'string=))
  (main))

