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

class I2_CIB_API CIB
{
public:
	static void RequireInformation(InformationType type);
	static int GetInformationTypes(void);

	static void UpdateTaskStatistics(long tv, int num);
	static int GetTaskStatistics(long timespan);

	static boost::signal<void (const ServiceStatusMessage&)> OnServiceStatusUpdate;

private:
	static int m_Types;

	static RingBuffer m_TaskStatistics;
};

}

#endif /* CIB_H */
