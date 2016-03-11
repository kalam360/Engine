/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2016 Quaternion Risk Management Ltd.
*/

/*! \file irlgm1fpiecewiselinearparametrization.hpp
    \brief piecewise linear model parametrization
*/

#ifndef quantext_piecewiselinear_irlgm1f_parametrization_hpp
#define quantext_piecewiselinear_irlgm1f_parametrization_hpp

#include <qle/models/irlgm1fparametrization.hpp>
#include <qle/models/piecewiseconstanthelper.hpp>

namespace QuantExt {

/*! parametrization with piecewise linear H and zeta,
    w.r.t. zeta this is the same as piecewise constant alpha,
    w.r.t. H this is implemented with a new (helper) parameter
    h > 0, such that H(t) = \int_0^t h(s) ds

    \warning this class is considered experimental, it is not
             tested well and might have conceptual issues
             (e.g. kappa is zero almost everywhere); you
             might rather want to rely on the piecewise
             constant parametrization
*/

template <class TS>
class Lgm1fPiecewiseLinearParametrization : public IrLgm1fParametrization,
                                            private PiecewiseConstantHelper11 {
  public:
    Lgm1fPiecewiseLinearParametrization(const Currency &currency,
                                        const Handle<TS> &termStructure,
                                        const Array &alphaTimes,
                                        const Array &alpha, const Array &hTimes,
                                        const Array &h);
    Lgm1fPiecewiseLinearParametrization(const Currency &currency,
                                        const Handle<TS> &termStructure,
                                        const std::vector<Date> &alphaDates,
                                        const Array &alpha,
                                        const std::vector<Date> &hDates,
                                        const Array &h);
    Real zeta(const Time t) const;
    Real H(const Time t) const;
    Real alpha(const Time t) const;
    Real kappa(Time t) const;
    Real Hprime(const Time t) const;
    Real Hprime2(const Time t) const;
    const Array &parameterTimes(const Size) const;
    const boost::shared_ptr<Parameter> parameter(const Size) const;
    void update() const;

  protected:
    Real direct(const Size i, const Real x) const;
    Real inverse(const Size j, const Real y) const;

  private:
    void initialize(const Array &alpha, const Array &h);
};

// implementation

template <class TS>
Lgm1fPiecewiseLinearParametrization<TS>::Lgm1fPiecewiseLinearParametrization(
    const Currency &currency, const Handle<TS> &termStructure,
    const Array &alphaTimes, const Array &alpha, const Array &hTimes,
    const Array &h)
    : IrLgm1fParametrization(currency, termStructure),
      PiecewiseConstantHelper11(alphaTimes, hTimes) {
    initialize(alpha, h);
}

template <class TS>
Lgm1fPiecewiseLinearParametrization<TS>::Lgm1fPiecewiseLinearParametrization(
    const Currency &currency, const Handle<TS> &termStructure,
    const std::vector<Date> &alphaDates, const Array &alpha,
    const std::vector<Date> &hDates, const Array &h)
    : IrLgm1fParametrization(currency, termStructure),
      PiecewiseConstantHelper11(alphaDates, hDates, termStructure) {
    initialize(alpha, h);
}

template <class TS>
void Lgm1fPiecewiseLinearParametrization<TS>::initialize(const Array &alpha,
                                                         const Array &h) {
    QL_REQUIRE(helper1().t().size() + 1 == alpha.size(),
               "alpha size (" << alpha.size()
                              << ") inconsistent to times size ("
                              << helper1().t().size() << ")");
    QL_REQUIRE(helper2().t().size() + 1 == h.size(),
               "h size (" << h.size() << ") inconsistent to times size ("
                          << helper1().t().size() << ")");
    // store raw parameter values
    for (Size i = 0; i < helper1().p()->size(); ++i) {
        helper1().p()->setParam(i, inverse(0, alpha[i]));
    }
    for (Size i = 0; i < helper2().p()->size(); ++i) {
        helper2().p()->setParam(i, inverse(1, h[i]));
    }
    update();
}

// inline

template <class TS>
inline Real
Lgm1fPiecewiseLinearParametrization<TS>::direct(const Size i,
                                                const Real x) const {
    return i == 0 ? helper1().direct(x) : helper2().direct(x);
}

template <class TS>
inline Real
Lgm1fPiecewiseLinearParametrization<TS>::inverse(const Size i,
                                                 const Real y) const {
    return i == 0 ? helper1().inverse(y) : helper2().inverse(y);
}

template <class TS>
inline Real Lgm1fPiecewiseLinearParametrization<TS>::zeta(const Time t) const {
    return helper1().int_y_sqr(t) / (scaling_ * scaling_);
}

template <class TS>
inline Real Lgm1fPiecewiseLinearParametrization<TS>::H(const Time t) const {
    return scaling_ * helper2().int_y_sqr(t) + shift_;
}

template <class TS>
inline Real Lgm1fPiecewiseLinearParametrization<TS>::alpha(const Time t) const {
    return helper1().y(t) / scaling_;
}

template <class TS>
inline Real Lgm1fPiecewiseLinearParametrization<TS>::kappa(const Time) const {
    return 0.0; // almost everywhere
}

template <class TS>
inline Real
Lgm1fPiecewiseLinearParametrization<TS>::Hprime(const Time t) const {
    return scaling_ * helper2().y(t);
}

template <class TS>
inline Real Lgm1fPiecewiseLinearParametrization<TS>::Hprime2(const Time) const {
    return 0.0; // almost everywhere
}

template <class TS>
inline void Lgm1fPiecewiseLinearParametrization<TS>::update() const {
    helper1().update();
    helper2().update();
}

template <class TS>
inline const Array &
Lgm1fPiecewiseLinearParametrization<TS>::parameterTimes(const Size i) const {
    QL_REQUIRE(i < 2, "parameter " << i << " does not exist, only have 0..1");
    if (i == 0)
        return helper1().t();
    else
        return helper2().t();
    ;
}

template <class TS>
inline const boost::shared_ptr<Parameter>
Lgm1fPiecewiseLinearParametrization<TS>::parameter(const Size i) const {
    QL_REQUIRE(i < 2, "parameter " << i << " does not exist, only have 0..1");
    if (i == 0)
        return helper1().p();
    else
        return helper2().p();
}

// typedef

typedef Lgm1fPiecewiseLinearParametrization<YieldTermStructure>
    IrLgm1fPiecewiseLinearParametrization;

} // namespace QuantExt

#endif
