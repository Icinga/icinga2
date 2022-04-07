/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef OPENTSDBWRITER_H
#define OPENTSDBWRITER_H

#include "perfdata/opentsdbwriter-ti.hpp"
#include "icinga/service.hpp"
#include "base/configobject.hpp"
#include "base/tcpsocket.hpp"
#include "base/timer.hpp"
#include <fstream>

namespace icinga
{

/**
 * An Icinga opentsdb writer.
 *
 * @ingroup perfdata
 */
class OpenTsdbWriter final : public ObjectImpl<OpenTsdbWriter>
{
public:
	DECLARE_OBJECT(OpenTsdbWriter);
	DECLARE_OBJECTNAME(OpenTsdbWriter);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	void ValidateHostTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;
	void ValidateServiceTemplate(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;

protected:
	void OnConfigLoaded() override;
	void Resume() override;
	void Pause() override;

private:
	Shared<AsioTcpStream>::Ptr m_Stream;

	boost::signals2::connection m_HandleCheckResults;
	Timer::Ptr m_ReconnectTimer;

	Dictionary::Ptr m_ServiceConfigTemplate;
	Dictionary::Ptr m_HostConfigTemplate;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void SendMetric(const Checkable::Ptr& checkable, const String& metric,
		const std::map<String, String>& tags, double value, double ts);
	void SendPerfdata(const Checkable::Ptr& checkable, const String& metric,
		const std::map<String, String>& tags, const CheckResult::Ptr& cr, double ts);
	static String EscapeTag(const String& str);
	static String EscapeMetric(const String& str);

	void ReconnectTimerHandler();

	void ReadConfigTemplate(const Dictionary::Ptr& stemplate, 
		const Dictionary::Ptr& htemplate);
};

}

#endif /* OPENTSDBWRITER_H */
