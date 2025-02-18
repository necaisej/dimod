.. _cqm:

=============================
Quadratic Models: Constrained
=============================

Class
=====

.. currentmodule:: dimod

.. autoclass:: ConstrainedQuadraticModel

.. autoclass:: CQM

Attributes
----------

.. autosummary::
   :toctree: generated/

   ConstrainedQuadraticModel.constraints
   ConstrainedQuadraticModel.objective
   ConstrainedQuadraticModel.variables

Methods
-------

.. autosummary::
   :toctree: generated/

   ConstrainedQuadraticModel.add_constraint
   ConstrainedQuadraticModel.add_constraint_from_comparison
   ConstrainedQuadraticModel.add_constraint_from_iterable
   ConstrainedQuadraticModel.add_constraint_from_model
   ConstrainedQuadraticModel.add_discrete
   ConstrainedQuadraticModel.add_variable
   ConstrainedQuadraticModel.check_feasible
   ConstrainedQuadraticModel.from_bqm
   ConstrainedQuadraticModel.from_discrete_quadratic_model
   ConstrainedQuadraticModel.from_dqm
   ConstrainedQuadraticModel.from_qm
   ConstrainedQuadraticModel.from_quadratic_model
   ConstrainedQuadraticModel.from_file
   ConstrainedQuadraticModel.iter_constraint_data
   ConstrainedQuadraticModel.iter_violations
   ConstrainedQuadraticModel.is_almost_equal
   ConstrainedQuadraticModel.is_equal
   ConstrainedQuadraticModel.num_biases
   ConstrainedQuadraticModel.num_quadratic_variables
   ConstrainedQuadraticModel.set_objective
   ConstrainedQuadraticModel.substitute_self_loops
   ConstrainedQuadraticModel.to_file
   ConstrainedQuadraticModel.vartype
   ConstrainedQuadraticModel.violations


Functions
=========

Converting constrained quadratic models to other model types:

.. currentmodule:: dimod

.. autosummary::
   :toctree: generated/

   cqm_to_bqm
