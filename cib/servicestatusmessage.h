#ifndef SERVICESTATUSMESSAGE_H
#define SERVICESTATUSMESSAGE_H

namespace icinga
{

class I2_CIB_API ServiceStatusMessage : public MessagePart
{
public:
	ServiceStatusMessage(void) : MessagePart() { }
	ServiceStatusMessage(const MessagePart& message) : MessagePart(message) { }

	bool GetService(string *service) const;
	void SetService(const string& service);

	bool GetState(ServiceState *state) const;
	void SetState(ServiceState state);

	bool GetStateType(ServiceStateType *type) const;
	void SetStateType(ServiceStateType type);

	bool GetCurrentCheckAttempt(long *attempt) const;
	void SetCurrentCheckAttempt(long attempt);

	bool GetNextCheck(time_t *ts) const;
	void SetNextCheck(time_t ts);

	bool GetCheckResult(CheckResult *cr) const;
	void SetCheckResult(CheckResult cr);
};

}

#endif /* SERVICESTATUSMESSAGE_H */
