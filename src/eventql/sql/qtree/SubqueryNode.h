/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>
#include <eventql/sql/svalue.h>
#include <eventql/sql/qtree/TableExpressionNode.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/SelectListNode.h>

#include "eventql/eventql.h"

namespace csql {

class SubqueryNode : public TableExpressionNode {
public:

  SubqueryNode(
      RefPtr<QueryTreeNode> subquery,
      Vector<RefPtr<SelectListNode>> select_list,
      Option<RefPtr<ValueExpressionNode>> where_expr);

  SubqueryNode(const SubqueryNode& other);

  RefPtr<QueryTreeNode> subquery() const;

  Vector<RefPtr<SelectListNode>> selectList() const;
  Vector<String> getResultColumns() const override;

  Vector<QualifiedColumn> getAvailableColumns() const override;

  size_t getComputedColumnIndex(
      const String& column_name,
      bool allow_add = false) override;

  size_t getNumComputedColumns() const override;

  SType getColumnType(size_t idx) const override;

  Option<RefPtr<ValueExpressionNode>> whereExpression() const;

  RefPtr<QueryTreeNode> deepCopy() const override;

  String toString() const override;

  const String& tableAlias() const;
  void setTableAlias(const String& alias);

  static void encode(
      QueryTreeCoder* coder,
      const SubqueryNode& node,
      OutputStream* os);

  static RefPtr<QueryTreeNode> decode(
      QueryTreeCoder* coder,
      InputStream* is);

protected:
  RefPtr<QueryTreeNode> subquery_;
  Vector<String> column_names_;
  Vector<RefPtr<SelectListNode>> select_list_;
  Option<RefPtr<ValueExpressionNode>> where_expr_;
  String alias_;
};

} // namespace csql
