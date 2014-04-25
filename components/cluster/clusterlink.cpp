/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "cluster/clusterlink.h"

using namespace icinga;

ClusterLink::ClusterLink(const String& from, const String& to)
{
	if (from < to) {
		From = from;
		To = to;
	} else {
		From = to;
		To = from;
	}
}

int ClusterLink::GetMetric(void) const
{
	int metric = 0;

	Endpoint::Ptr fromEp = Endpoint::GetByName(From);
	if (fromEp)
		metric += fromEp->GetMetric();

	Endpoint::Ptr toEp = Endpoint::GetByName(To);
	if (toEp)
		metric += toEp->GetMetric();

	return metric;
}

bool ClusterLink::operator<(const ClusterLink& other) const
{
	if (From < other.From)
		return true;
	else
		return To < other.To;
}

bool ClusterLinkMetricLessComparer::operator()(const ClusterLink& a, const ClusterLink& b) const
{
	int metricA = a.GetMetric();
	int metricB = b.GetMetric();

	if (metricA < metricB)
		return true;
	else if (metricB > metricA)
		return false;
	else
		return a < b;
}
