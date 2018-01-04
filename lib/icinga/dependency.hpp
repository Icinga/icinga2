/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#include "icinga/i2-icinga.hpp"
#include "icinga/dependency.thpp"

namespace icinga
{

class ApplyRule;
struct ScriptFrame;
class Host;
class Service;

/**
 * A service dependency..
 *
 * @ingroup icinga
 */
class Dependency final : public ObjectImpl<Dependency>
{
public:
	DECLARE_OBJECT(Dependency);
	DECLARE_OBJECTNAME(Dependency);

	intrusive_ptr<Checkable> GetParent() const;
	intrusive_ptr<Checkable> GetChild() const;

	TimePeriod::Ptr GetPeriod() const;

	bool IsAvailable(DependencyType dt) const;

	void ValidateStates(const Array::Ptr& value, const ValidationUtils& utils) override;

	static void EvaluateApplyRules(const intrusive_ptr<Host>& host);
	static void EvaluateApplyRules(const intrusive_ptr<Service>& service);

protected:
	void OnConfigLoaded() override;
	void OnAllConfigLoaded() override;
	void Stop(bool runtimeRemoved) override;

private:
	Checkable::Ptr m_Parent;
	Checkable::Ptr m_Child;

	static bool EvaluateApplyRuleInstance(const Checkable::Ptr& checkable, const String& name, ScriptFrame& frame, const ApplyRule& rule);
	static bool EvaluateApplyRule(const Checkable::Ptr& checkable, const ApplyRule& rule);
};

}

#endif /* DEPENDENCY_H */
