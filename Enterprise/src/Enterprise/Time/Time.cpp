#include "EP_PCH.h"
#include "Time.h"

// Tick() vars

/// Time, in real seconds, since the application started.
float runningTime = 0.0f;
/// The previous value of runningTime.
float prevTime = 0.0f;
/// The current conversion rate between real seconds and game seconds.
float timeScale = 1.0f;
/// The number of real seconds that have passed since last Tick().
float tickDeltaReal = 0.0f;
/// The amount of game-time seconds that have passed since last Tick().
float tickDeltaScaled = 0.0f;


// Accumulators

float frameAccumulator = 0.0f; float frameAccumulator_real = 0.0f;
float physFrameAccumulator = Enterprise::Constants::PhysFrameLength; float physFrameAccumulator_real = 0.0f;
float physFrameRepeatAccumulator = 0.0f;


// Gettable vars

/// The number of game-seconds being simulated this frame or physics frame.
float frameDelta = 0.0f;
/// The number of real seconds the current frame or physics frame represents.
float realDelta = 0.0f;
/// A value in [0.0, 1.0] representing the current progress through the physics frame.
float physPhase = 1.0f;

namespace Enterprise
{
	float Time::RunningTime() { return runningTime; }
	float Time::FrameDelta() { return frameDelta; }
	float Time::RealDelta() { return realDelta; }
	float Time::PhysPhase() { EP_ASSERT(physPhase > 0.0f); return physPhase; }
	
    void Time::SetTimeScale(float scalar)
	{
		// TODO: Make it so negative values don't crash the game.
		EP_ASSERT(scalar >= 0.0f); //Time::SetTimeScale() called with negative parameter.  Scalar cannot be negative.  Set timeScale to 0.0f.
		timeScale = scalar;
	}

    // ------------------------------------------------------------------

	void Time::Tick()
	{
		prevTime = runningTime;
		runningTime = GetRawTime();
		tickDeltaReal = runningTime - prevTime;
		tickDeltaScaled = tickDeltaReal * timeScale;

		// Increment accumulators
		frameAccumulator += tickDeltaScaled;
		frameAccumulator_real += tickDeltaReal;
		physFrameAccumulator += tickDeltaScaled;
		physFrameAccumulator_real += tickDeltaReal;
		physFrameRepeatAccumulator += tickDeltaReal;

		// Perhaps here is the place to increment timers?
	}

	bool Time::PhysFrame()
	{
		Tick();

		// Abort death spirals
		if (physFrameRepeatAccumulator >= Constants::PhysFrameRepeatCap)
		{
            EP_WARN("Time: Physics frames were skipped to abort a death spiral. \n"
                    "Accumulator: {}\nCap: {}\nFrames Dropped: {}", 
					physFrameRepeatAccumulator, Constants::PhysFrameRepeatCap, 
					(physFrameRepeatAccumulator - Constants::PhysFrameRepeatCap) / Constants::PhysFrameLength);

			// Dump remaining time from accumulators
			frameAccumulator -= (physFrameAccumulator - Constants::PhysFrameLength);
			physFrameAccumulator = Constants::PhysFrameLength;
			physPhase = 1.0f;
			
			// Move to a new general frame
			return false;
			
			// physFrameRepeatAccumulator gets reset in FrameEnd().
		}

		// Check the PhysFrame timer
		if (physFrameAccumulator >= Constants::PhysFrameLength)
		{
			// Update gettable vars
			frameDelta = Constants::PhysFrameLength;
			realDelta = physFrameAccumulator_real;
			physPhase = -1.0f; // HACK: Triggers assertion if physics code calls PhysPhase().

			// Reset accumulators
			physFrameAccumulator -= Constants::PhysFrameLength;
			physFrameAccumulator_real = 0.0f;

			// Trigger PhysFrame
			return true;
		}
		else
			return false;
	}

	void Time::FrameStart()
	{
		Tick();

		// Update gettable vars
		frameDelta = frameAccumulator;
		realDelta = frameAccumulator_real;
		physPhase = physFrameAccumulator / Constants::PhysFrameLength;

		// Reset accumulator
		frameAccumulator = 0.0f;
		frameAccumulator_real = 0.0f;
	}

	void Time::FrameEnd()
	{
		physFrameRepeatAccumulator = 0.0f;
	}
}
