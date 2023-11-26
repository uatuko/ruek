#include "principals.h"

namespace svc {
Principals::Principals(grpcxx::server &s) : _svc(*this) {
	s.add(_svc);
}
} // namespace svc
