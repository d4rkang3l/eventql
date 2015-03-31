/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "CTRByPositionServlet.h"
#include "CTRCounter.h"
#include "fnord-base/Language.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/io/fileutil.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/SSTableScan.h"

using namespace fnord;

namespace cm {

CTRByPositionServlet::CTRByPositionServlet(VFS* vfs) : vfs_(vfs) {}

void CTRByPositionServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  auto end_time = WallClock::unixMicros();
  auto start_time = end_time - 30 * kMicrosPerDay;
  Set<String> test_groups;
  Set<String> device_types;

  /* arguments */
  String customer;
  if (!URI::getParam(params, "customer", &customer)) {
    res->addBody("error: missing ?customer=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  String lang_str;
  if (!URI::getParam(params, "lang", &lang_str)) {
    res->addBody("error: missing ?lang=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  for (const auto& p : params) {
    if (p.first == "from") {
      start_time = std::stoul(p.second) * kMicrosPerSecond;
      continue;
    }

    if (p.first == "until") {
      end_time = std::stoul(p.second) * kMicrosPerSecond;
      continue;
    }

    if (p.first == "test_group") {
      test_groups.emplace(p.second);
      continue;
    }

    if (p.first == "device_type") {
      device_types.emplace(p.second);
      continue;
    }

  }

  if (test_groups.size() == 0) {
    test_groups.emplace("all");
  }

  if (device_types.size() == 0) {
    device_types = Set<String> { "unknown", "desktop", "tablet", "phone" };
  }

  /* prepare prefix filters for scanning */
  String scan_common_prefix = lang_str + "~";
  if (test_groups.size() == 1) {
    scan_common_prefix += *test_groups.begin() + "~";

    if (device_types.size() == 1) {
      scan_common_prefix += *device_types.begin() + "~";
    }
  }

  /* prepare input tables */
  Set<String> tables;
  for (uint64_t i = end_time; i >= start_time; i -= kMicrosPerDay) {
    tables.emplace(StringUtil::format(
        "$0_ctr_by_position_daily.$1.sstable",
        customer,
        i / kMicrosPerDay));
  }

  /* scan input tables */
  HashMap<uint64_t, CTRCounterData> counters;
  for (const auto& tbl : tables) {
    if (!vfs_->exists(tbl)) {
      continue;
    }

    sstable::SSTableReader reader(vfs_->openFile(tbl));
    if (reader.bodySize() == 0 || reader.isFinalized() == 0) {
      continue;
    }

    sstable::SSTableScan scan;
    scan.setKeyPrefix(scan_common_prefix);

    auto cursor = reader.getCursor();
    scan.execute(cursor.get(), [&] (const Vector<String> row) {
      if (row.size() != 2) {
        RAISEF(kRuntimeError, "invalid row length: $0", row.size());
      }

      auto dims = StringUtil::split(row[0], "~");
      if (dims.size() != 4) {
        RAISEF(kRuntimeError, "invalid row key: $0", row[0]);
      }

      if (dims[0] != lang_str) {
        return;
      }

      if (test_groups.count(dims[1]) == 0) {
        return;
      }

      if (device_types.count(dims[2]) == 0) {
        return;
      }

      auto counter = CTRCounterData::load(row[1]);
      auto posi = std::stoul(dims[3]);

      counters[posi].merge(counter);
    });
  }


  /* write response */
  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream json(res->getBodyOutputStream());

  json.beginArray();

  uint64_t total_views = 0;
  uint64_t total_clicks = 0;

  for (const auto& c : counters) {
    total_views += c.second.num_views;
    total_clicks += c.second.num_clicks;
  }

  int n = 0;
  for (const auto& c : counters) {
    if (++n > 1) {
      json.addComma();
    }

    json.beginObject();
    json.addObjectEntry("position");
    json.addInteger(c.first);
    json.addComma();
    json.addObjectEntry("views");
    json.addInteger(c.second.num_views);
    json.addComma();
    json.addObjectEntry("clicks");
    json.addInteger(c.second.num_clicks);
    json.addComma();
    json.addObjectEntry("ctr");
    json.addFloat(c.second.num_clicks / (double) c.second.num_views);
    json.addComma();
    json.addObjectEntry("ctr_base");
    json.addFloat(c.second.num_clicks / (double) total_views);
    json.addComma();
    json.addObjectEntry("clickshare");
    json.addFloat(c.second.num_clicks / (double) total_clicks);
    json.endObject();
  }

  json.endArray();
}

}
