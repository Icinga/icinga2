#ifndef CIB_H
#define CIB_H

namespace icinga
{

enum InformationType
{
	CIB_Configuration = 1<<0,
	CIB_ProgramStatus = 1<<1,
	CIB_ServiceStatus = 1<<2
};

class CIB
{
public:
	static void RequireInformation(InformationType type);

	static void Start(void);

private:
	static int m_Types;
	static VirtualEndpoint::Ptr m_Endpoint;

	static void ServiceStatusRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request);
};

}

#endif /* CIB_H */
