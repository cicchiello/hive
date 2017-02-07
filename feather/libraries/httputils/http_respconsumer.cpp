#include <http_respconsumer.h>


HttpResponseConsumer::HttpResponseConsumer(const WifiUtils::Context &ctxt)
  : m_ctxt(ctxt), m_timeout(10000 /*10s*/)
{
}


void HttpResponseConsumer::reset()
{
}
