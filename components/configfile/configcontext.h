#ifndef CONFIGCONTEXT_H
#define CONFIGCONTEXT_H

class ConfigContext
{
public:
	ConfigContext(std::istream *input = &std::cin);
	virtual ~ConfigContext(void);

	std::istream *Input;
	void *Scanner;

private:
	void InitializeScanner(void);
	void DestroyScanner(void);
};

int icingaparse(ConfigContext *context);

#endif /* CONFIGCONTEXT_H */
