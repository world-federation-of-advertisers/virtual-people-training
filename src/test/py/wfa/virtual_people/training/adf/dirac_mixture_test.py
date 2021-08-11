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

"""Tests for dirac_mixture."""


from absl.testing import absltest

from src.main.py.wfa.virtual_people.training.adf import dirac_mixture
import numpy


class DiracMixtureTest(absltest.TestCase):

  def test_fit_alphas(self):
    signal = numpy.random.uniform(0, 1, (1000, 2))
    ds = numpy.array([[1.5, 0.5],
                      [0.5, 1.5]])
    true_alphas = [0.2, 0.8]
    true_dm = dirac_mixture.DiracMixture(true_alphas, ds)
    target = true_dm(signal)
    fitted_alphas = dirac_mixture.fit_alphas(ds, signal, target)
    self.assertSequenceAlmostEqual(true_alphas, fitted_alphas.tolist())

  def test_fit_adaptive_dirac_mixture(self):
    """Testing that ADM trianing can fit curve perfectly."""
    signal = numpy.random.uniform(0, 1, (1000, 2))
    ds = numpy.array([[1.5, 0.5],
                      [0.5, 1.5],
                      [0.8, 1.2]])
    true_alphas = [0.2, 0.3, 0.5]
    true_dm = dirac_mixture.DiracMixture(true_alphas, ds)
    target = true_dm(signal)

    adm = dirac_mixture.AdaptiveDiracMixture()
    adm.fit(signal, target)
    prediction = adm(signal)
    self.assertSequenceAlmostEqual(target, prediction)
    distance = numpy.linalg.norm(target - prediction)
    self.assertLess(distance, 0.01)

  def test_fit_adaptive_dirac_mixture_insufficient_steps(self):
    """Testing that small number of steps is insufficient for perfect fit."""
    signal = numpy.random.uniform(0, 1, (1000, 2))
    ds = numpy.array([[1.5, 0.5],
                      [0.5, 1.5],
                      [0.8, 1.2]])
    true_alphas = [0.3, 0.3, 0.4]
    true_dm = dirac_mixture.DiracMixture(true_alphas, ds)
    target = true_dm(signal)

    adm = dirac_mixture.AdaptiveDiracMixture()
    adm.fit(signal, target, max_steps=10)
    prediction = adm(signal)

    distance = numpy.linalg.norm(target - prediction)
    self.assertGreater(distance, 0.1)


if __name__ == '__main__':
  absltest.main()
