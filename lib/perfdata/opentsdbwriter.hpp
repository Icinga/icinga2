/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef OPENTSDBWRITER_H
#define OPENTSDBWRITER_H

#include "perfdata/opentsdbwriter-ti.hpp"
#include "icinga/checkable.hpp"
#include "base/configobject.hpp"
#include "perfdata/perfdatawriterconnection.hpp"

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
	WorkQueue m_WorkQueue{10000000, 1};
	std::string m_MsgBuf;
	PerfdataWriterConnection::Ptr m_Connection;

	boost::signals2::connection m_HandleCheckResults;

	Dictionary::Ptr m_ServiceConfigTemplate;
	Dictionary::Ptr m_HostConfigTemplate;

	void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);
	void AddMetric(const Checkable::Ptr& checkable, const String& metric,
		const std::map<String, String>& tags, double value, double ts);
	void SendMsgBuffer();
	void AddPerfdata(const Checkable::Ptr& checkable, const String& metric,
		const std::map<String, String>& tags, const CheckResult::Ptr& cr, double ts);
	static String EscapeTag(const String& str);
	static String EscapeMetric(const String& str);

	void ReadConfigTemplate();
};

}

#endif /* OPENTSDBWRITER_H */
