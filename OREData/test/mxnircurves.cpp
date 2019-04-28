/*
 Copyright (C) 2019 Quaternion Risk Management Ltd
 All rights reserved.

 This file is part of ORE, a free-software/open-source library
 for transparent pricing and risk analysis - http://opensourcerisk.org

 ORE is free software: you can redistribute it and/or modify it
 under the terms of the Modified BSD License.  You should have received a
 copy of the license along with this program.
 The license is also available online at <http://opensourcerisk.org>

 This program is distributed on the basis that it will form a useful
 contribution to risk analytics and model standardisation, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
*/

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <ored/configuration/conventions.hpp>
#include <ored/configuration/curveconfigurations.hpp>
#include <ored/marketdata/csvloader.hpp>
#include <ored/marketdata/todaysmarketparameters.hpp>
#include <ored/marketdata/todaysmarket.hpp>
#include <ored/portfolio/enginedata.hpp>
#include <ored/portfolio/enginefactory.hpp>
#include <ored/portfolio/portfolio.hpp>
#include <oret/datapaths.hpp>
#include <oret/toplevelfixture.hpp>

#include <ql/time/calendar.hpp>
#include <ql/time/daycounter.hpp>

using namespace ore::data;
using namespace QuantLib;
using namespace boost::unit_test_framework;

using ore::test::TopLevelFixture;
using std::ostream;

BOOST_FIXTURE_TEST_SUITE(OREDataTestSuite, TopLevelFixture)

BOOST_AUTO_TEST_SUITE(MXNIrCurvesTest)

// This is mainly a check that the schedule gets built correctly given that MXN-TIIE has a 28D tenor
BOOST_AUTO_TEST_CASE(testYieldCurveBootstrap) {

    // Evaluation date
    Date asof(17, Apr, 2019);
    Settings::instance().evaluationDate() = asof;

    // Market
    Conventions conventions;
    conventions.fromFile(TEST_INPUT_FILE("conventions_01.xml"));
    TodaysMarketParameters todaysMarketParams;
    todaysMarketParams.fromFile(TEST_INPUT_FILE("todaysmarket_01.xml"));
    CurveConfigurations curveConfigs;
    curveConfigs.fromFile(TEST_INPUT_FILE("curveconfig_01.xml"));
    CSVLoader loader({ TEST_INPUT_FILE("market_01.txt") }, { TEST_INPUT_FILE("fixings.txt") }, false);
    boost::shared_ptr<TodaysMarket> market = boost::make_shared<TodaysMarket>(
        asof, todaysMarketParams, loader, curveConfigs, conventions, false);

    // Portfolio to test market
    boost::shared_ptr<EngineData> engineData = boost::make_shared<EngineData>();
    engineData->fromFile(TEST_INPUT_FILE("pricingengine_01.xml"));
    boost::shared_ptr<EngineFactory> factory = boost::make_shared<EngineFactory>(engineData, market);
    boost::shared_ptr<Portfolio> portfolio = boost::make_shared<Portfolio>();
    portfolio->load(TEST_INPUT_FILE("mxn_ir_swap.xml"));
    portfolio->build(factory);

    // The single trade in the portfolio is a MXN 10Y swap, i.e. 10 x 13 28D coupons, with nominal 100 million. The 
    // rate on the swap is equal to the 10Y rate in the market file 'market_01.txt' so we should get an NPV of 0.
    BOOST_CHECK_EQUAL(portfolio->size(), 1);
    BOOST_CHECK_SMALL(portfolio->trades()[0]->instrument()->NPV(), 0.01);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
