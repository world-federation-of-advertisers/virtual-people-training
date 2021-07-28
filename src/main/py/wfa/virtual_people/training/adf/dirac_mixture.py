#!/usr/bin/python
#
# Copyright 2021 The Cross-Media Measurement Authors 
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Library for fitting Dirac Mixture reach models.

Implementing Adaptive Dirac Mixture fitting algorithm.
It is Algorithm 4 from paper
Measuring Cross-Device Online Audiences by
Jim Koehler, Evgeny Skvortsov, Sheng Ma and Song Liu.
Link: https://research.google/pubs/pub45353/

Example usage:
# signal is n x m matrix, where each row is a vector of reaches into types of
# user ids. E.g. each row could be [desktop_cookie_reach, mobile_cookie_reach].
# target is a column vector of dimensionality n, where each element is the
# number of people reached.
adm = AdaptiveDiracMixture()
adm.fit(signal, target)
# Now adm.ds are centers of dirac deltas and adm.alphas are coefficients.
"""

import numpy

from scipy import optimize


def fit_alphas(ds, signal, target):
  """Fitting optimal coefficients for deltas as fixed positions.

  Args:
    ds: Dirac Delta locations.
    signal: Array, where each row is vector of counts of various types of
      userids reached.
    target: Array vector, where each element is the number of people reached.

  Returns:
    An array of coefficients for the Dirac Deltas approximating the reach.
  """
  c = DiracMixture.linear_basis(signal, ds).T
  result, _ = optimize.nnls(c, target, maxiter=1000)
  return result


class DiracMixture(object):
  """Implements Dirac Mixture."""

  def __init__(self, alphas, ds):
    """Instantiating Dirac Mixture.

    Args:
      alphas: A 1d numpy array or a list of delta coefficients.
      ds: A 2d numpy array of Dirac Delta locations.
    """
    # Delayed instantiation is allowed.
    if alphas is not None and ds is not None:
      # Checking compatibility of shapes of alphas and ds.
      assert len(ds.shape) == 2
      if isinstance(alphas, list):
        assert ds.shape[0] == len(alphas), (
                'Inconsistent shapes: alphas=%s, ds=%s' % (alphas, ds))
      elif isinstance(alphas, numpy.ndarray):
        assert len(alphas.shape) == 1
        assert len(ds.shape[1]) == alphas.shape[0]
      else:
        assert False, ('Dirac mixture alpha coefficients must be '
                       'a list or an array.')
    self.alphas = alphas
    self.ds = ds

  def __str__(self):
    return ('+'.join(
        '%.3f * Delta(%s)' % (alpha, delta)
        for alpha, delta in zip(self.alphas, self.ds)))

  @classmethod
  def linear_basis(cls, signal, ds):
    """Signal specific linear basis corresponding delta locations ds.

    Consider a set of audiences that are recorded in the signal.
    A Dirac Mixture can be seen as a vector in this space, which i-th
    coordinate is reach of i-th audience.
    For given signal location of Dirac Deltas all Dirac Mixtures form
    a linear cone. This functions returns basis of the cone.
    Args:
      signal: 2d numpy array, where each row is a vector of userid
        counts corresponding to an audience.
      ds: 2d numpy array, where each row is a vector of Dirac Delta
        coordinates.

    Returns:
      A matrix, where i-th row is reach of i-th delta on each audience
      of the signal.
    """
    return 1 - numpy.exp(- ds.dot(signal.T))

  @classmethod
  def reach(cls, alphas, ds, signal):
    return numpy.hstack(alphas).dot(
        cls.linear_basis(signal, ds))

  def __call__(self, signal):
    """Applying Dirac Mixture to a collection of audiences.

    Args:
      signal: A 2d numpy array, each row is observed counts of user
      identifiers at a particular audience.

    Returns:
      1d numpy array, where i-th number is reach of campaign
      corresponding to i-th row of signal.
    """
    return self.reach(self.alphas, self.ds, signal)


def sample_from_gaussian_mixture(centers, center_probabilities,
                                 num_new_centers, sigma):
  """Sampling from mixture of Gaussians of the same symmetric variance.

  Args:
    centers: Centers of Gaussians.
    center_probabilities: Probability of each center being used for sampling.
    num_new_centers: Number of new centers to sample.
    sigma: Standard deviation defining the symmetric Gaussians.

  Returns:
    Points sampled from this mixture.
  """
  dim = centers.shape[1]
  new_centers = centers[numpy.random.choice(
      len(center_probabilities),
      num_new_centers,
      replace=True, p=center_probabilities)]
  noise = numpy.random.normal(0, sigma, (num_new_centers, dim))
  new_centers += noise
  min_coord = new_centers.min(axis=1)
  sum_coord = new_centers.sum(axis=1)
  new_centers = new_centers[(min_coord > 0) & (sum_coord > 0)]
  return new_centers


def fit_adaptive_dirac_mixture(signal, target,
                               new_centers_at_each_step,
                               new_centers_sigma,
                               max_steps,
                               initial_centers,
                               iteration_callback):
  """Internal function fitting to an observed cloud.

  Args:
    signal: Array, where each row is vector of counts of various types of
      userids reached.
    target: Array vector, where each element is the number of people reached.
    new_centers_at_each_step: Number of new Dirac Delta centers added at each
      step.
    new_centers_sigma: Standard deviation of centers around existing centers.
    max_steps: Maximal number of steps for the algorithm to run.
    initial_centers: Optional positions of initial centers. If not specified
      then one center of all-ones is used.
    iteration_callback: A function of arguments (iteration_number, alphas, ds),
      which will be called at each step of the algorithm. Can be used to record
      or display trajectory of the search.

  Returns:
    Coefficients and locations of Dirac Deltas.
  """
  dim = signal.shape[1]
  centers = initial_centers or numpy.vstack([numpy.ones(dim)])
  alphas = numpy.ones(len(centers)) / len(centers)
  for i in range(max_steps):
    # Sampling new centers and adding them to the mix.
    center_probabilities = alphas / sum(alphas)
    new_centers = sample_from_gaussian_mixture(
        centers, center_probabilities,
        new_centers_at_each_step, new_centers_sigma)
    centers = numpy.vstack([centers, new_centers])
    # Fitting coefficients of the Dirac Deltas.
    alphas = fit_alphas(centers,
                        signal,
                        target)
    # Removing centers with zero weight.
    centers = centers[alphas > 0]
    alphas = alphas[alphas > 0]
    # Calling user's callback for custom monitoring.
    if iteration_callback:
      iteration_callback(i, alphas, centers)
  return alphas, centers


class AdaptiveDiracMixture(DiracMixture):
  """Dirac Mixture that can be fit using ADM algorithm."""

  def __init__(self):
    super().__init__(None, None)

  def fit(self, signal, target,
          new_centers_at_each_step=10,
          new_centers_sigma=0.01,
          max_steps=1000,
          initial_centers=None,
          iteration_callback=None):
    """Fitting Adaptive Dirac Mixture."""
    self.alphas, self.ds = fit_adaptive_dirac_mixture(
        signal, target,
        new_centers_at_each_step=new_centers_at_each_step,
        new_centers_sigma=new_centers_sigma,
        max_steps=max_steps,
        initial_centers=initial_centers,
        iteration_callback=iteration_callback)
