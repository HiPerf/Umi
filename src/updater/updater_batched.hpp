#pragma once

#include "updater/updater.hpp"


template <typename... types>
class updater_batched : public updater<types...>
{

};
