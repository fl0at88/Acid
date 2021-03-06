﻿#pragma once

#include "Maths/Timer.hpp"

namespace acid
{
	class ACID_EXPORT InputDelay
	{
	private:
		static const Time DELAY_TIME;
		static const Time REPEAT_TIME;

		Timer m_timerDelay;
		Timer m_timerRepeat;
		bool m_delayOver;
	public:
		InputDelay();

		void Update(const bool &keyIsDown);

		bool CanInput();
	};
}
