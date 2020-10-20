#include <any>
#include <stdexcept>

// needed to support OS X < 10.14, because older libc++ does not have the necessary code for bad_any_cast
namespace std
{
	template<typename T, typename Any = any>
	T any_cast_(Any &&a)
	{
		if(auto &&result = any_cast<remove_reference_t<T>>(&a); result)
		{
			return *result;
		}

		throw runtime_error{"bad_any_cast"};
	}
}

#define any_cast any_cast_
