/*
 Copyright (C) 2023 Quaternion Risk Management Ltd
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

#include <orea/app/analytics/simmanalytic.hpp>
#include <orea/app/reportwriter.hpp>
#include <orea/simm/simmcalculator.hpp>
#include <orea/simm/utilities.hpp>

using namespace ore::data;
using namespace boost::filesystem;

namespace ore {
namespace analytics {

void SimmAnalyticImpl::setUpConfigurations() {
    analytic()->configurations().todaysMarketParams = inputs_->todaysMarketParams();
}

void SimmAnalyticImpl::runAnalytic(const boost::shared_ptr<ore::data::InMemoryLoader>& loader,
                                   const std::set<std::string>& runTypes) {
    if (!analytic()->match(runTypes))
        return;

    LOG("SimmAnalytic::runAnalytic called");

    analytic()->buildMarket(loader, false);

    auto simmAnalytic = static_cast<SimmAnalytic*>(analytic());
    QL_REQUIRE(simmAnalytic, "Analytic must be of type SimmAnalytic");

    LOG("Get CRIF records from CRIF loader and fill amountUSD");        
    simmAnalytic->loadCrifRecords(loader);

    if (analytic()->getWriteIntermediateReports()) {
        boost::shared_ptr<InMemoryReport> crifReport = boost::make_shared<InMemoryReport>();
        ReportWriter(inputs_->reportNaString()).writeCrifReport(crifReport, simmAnalytic->crif());
        analytic()->reports()[LABEL]["crif"] = crifReport;
        LOG("CRIF report generated");

        Crif simmDataCrif = simmAnalytic->crif().aggregate();
        boost::shared_ptr<InMemoryReport> simmDataReport = boost::make_shared<InMemoryReport>();
        ReportWriter(inputs_->reportNaString())
            .writeSIMMData(simmAnalytic->crif(), simmDataReport);
        analytic()->reports()[LABEL]["simm_data"] = simmDataReport;
        LOG("SIMM data report generated");
    }
    MEM_LOG;

    LOG("Calculating SIMM");

    // Save SIMM calibration data to output
    if (inputs_->simmCalibrationData())
        inputs_->simmCalibrationData()->toFile((inputs_->resultsPath() / "simmcalibration.xml").string());

    // Calculate SIMM
    auto simm = boost::make_shared<SimmCalculator>(simmAnalytic->crif(),
                                                   inputs_->getSimmConfiguration(),
                                                   inputs_->simmCalculationCurrency(),
                                                   inputs_->simmResultCurrency(),
                                                   analytic()->market(),
                                                   simmAnalytic->determineWinningRegulations(),
                                                   inputs_->enforceIMRegulations());

    Real fxSpot = 1.0;
    if (!inputs_->simmReportingCurrency().empty()) {
        fxSpot = analytic()->market()
            ->fxRate(inputs_->simmResultCurrency() + inputs_->simmReportingCurrency())
            ->value();
        LOG("SIMM reporting currency is " << inputs_->simmReportingCurrency() << " with fxSpot " << fxSpot);
    }

    boost::shared_ptr<InMemoryReport> simmRegulationBreakdownReport = boost::make_shared<InMemoryReport>();
    ReportWriter(inputs_->reportNaString())
        .writeSIMMReport(simm->simmResults(), simmRegulationBreakdownReport, simmAnalytic->hasNettingSetDetails(),
                         inputs_->simmResultCurrency(), inputs_->simmCalculationCurrency(),
                         inputs_->simmReportingCurrency(), false, fxSpot);
    LOG("SIMM regulation breakdown report generated");
    analytic()->reports()[LABEL]["regulation_breakdown_simm"] = simmRegulationBreakdownReport;


    boost::shared_ptr<InMemoryReport> simmReport = boost::make_shared<InMemoryReport>();
    ReportWriter(inputs_->reportNaString())
        .writeSIMMReport(simm->finalSimmResults(), simmReport, simmAnalytic->hasNettingSetDetails(),
                         inputs_->simmResultCurrency(), inputs_->simmCalculationCurrency(),
                         inputs_->simmReportingCurrency(), fxSpot);
    analytic()->reports()[LABEL]["simm"] = simmReport;
    LOG("SIMM report generated");
    MEM_LOG;

}

void SimmAnalytic::loadCrifRecords(const boost::shared_ptr<ore::data::InMemoryLoader>& loader) {
    QL_REQUIRE(inputs_, "Inputs not set");
    QL_REQUIRE(!inputs_->crif().empty(), "CRIF loader does not contain any records");
        
    crif_ = inputs_->crif();
    crif_.fillAmountUsd(market());
    hasNettingSetDetails_ = crif_.hasNettingSetDetails();
}

} // namespace analytics
} // namespace ore
