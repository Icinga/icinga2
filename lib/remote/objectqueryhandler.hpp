/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef OBJECTQUERYHANDLER_H
#define OBJECTQUERYHANDLER_H

#include "remote/httphandler.hpp"

namespace icinga
{

class ObjectQueryHandler final : public HttpHandler
{
public:
	DECLARE_PTR_TYPEDEFS(ObjectQueryHandler);

	bool HandleRequest(
		const WaitGroup::Ptr& waitGroup,
		const HttpApiRequest& request,
		HttpApiResponse& response,
		boost::asio::yield_context& yc
	) override;

private:
	static Dictionary::Ptr SerializeObjectAttrs(const Object::Ptr& object, const String& attrPrefix,
		const Array::Ptr& attrs, bool isJoin, bool allAttrs);
};

}

#endif /* OBJECTQUERYHANDLER_H */
