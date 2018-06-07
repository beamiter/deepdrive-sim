
#include "DeepDrivePluginPrivatePCH.h"
#include "Private/Simulation/Agent/Controllers/LocalAI/States/DeepDriveAgentPullOutState.h"
#include "Private/Simulation/Agent/Controllers/DeepDriveAgentSpeedController.h"
#include "Private/Simulation/Agent/Controllers/DeepDriveAgentSteeringController.h"
#include "Public/Simulation/Agent/Controllers/DeepDriveAgentLocalAIController.h"
#include "Public/Simulation/Agent/DeepDriveAgent.h"


DeepDriveAgentPullOutState::DeepDriveAgentPullOutState(DeepDriveAgentLocalAIStateMachine &stateMachine)
	: DeepDriveAgentLocalAIStateBase(stateMachine, "PullOut")
{

}


void DeepDriveAgentPullOutState::enter(DeepDriveAgentLocalAIStateMachineContext &ctx)
{
	m_PullOutTimeFactor = 1.0f / ctx.configuration.ChangeLaneDuration;
	m_PullOutAlpha = 0.0f;

	m_curOffset = 0.0f;

	startThinkTimer(ctx.configuration.ThinkDelays.Y, false);

	UE_LOG(LogDeepDriveAgentLocalAIController, Log, TEXT("Agent %d Pulling out"), ctx.agent.getAgentId());
}

void DeepDriveAgentPullOutState::update(DeepDriveAgentLocalAIStateMachineContext &ctx, float dT)
{
	m_PullOutAlpha += m_PullOutTimeFactor * dT;
	m_curOffset = FMath::Lerp(0.0f, ctx.configuration.OvertakingOffset, m_PullOutAlpha);

	float desiredSpeed = ctx.configuration.OvertakingSpeed;// ctx.local_ai_ctrl.getDesiredSpeed();
	const float limitedSpeed = ctx.speed_controller.limitSpeedByNextAgent(desiredSpeed);

	desiredSpeed = FMath::Lerp(limitedSpeed, desiredSpeed, FMath::SmoothStep(0.4f, 0.6f, m_PullOutAlpha));
	desiredSpeed = ctx.speed_controller.limitSpeedByTrack(desiredSpeed, ctx.configuration.SpeedLimitFactor);

	if (m_PullOutAlpha >= 1.0f)
	{
		m_StateMachine.setNextState("Passing");
	}
	else if(isTimeToThink(dT) && abortOvertaking(ctx, ctx.agent.getSpeedKmh()))
	{
		m_StateMachine.setNextState("PullBackIn");
	}

	ctx.speed_controller.update(dT, desiredSpeed);
	ctx.steering_controller.update(dT, desiredSpeed, m_curOffset);
}

void DeepDriveAgentPullOutState::exit(DeepDriveAgentLocalAIStateMachineContext &ctx)
{
	ctx.side_offset = m_curOffset;
}


bool DeepDriveAgentPullOutState::abortOvertaking(DeepDriveAgentLocalAIStateMachineContext &ctx, float desiredSpeed)
{
	bool abort = false;

	float distanceToNextAgent = -1.0f;
	ADeepDriveAgent *nextAgent = ctx.agent.getNextAgent(&distanceToNextAgent);
	if (nextAgent)
	{
		const float curSpeed = ctx.agent.getSpeedKmh();
		//const float speedDiff = (ctx.configuration.OvertakingSpeed - nextAgent->getSpeed() * 0.036f);
		const float speedDiff = FMath::Max(1.0f, (desiredSpeed - nextAgent->getSpeedKmh()));
		float nextButOneDist = -1.0f;
		ADeepDriveAgent *nextButOne = nextAgent->getNextAgent(&nextButOneDist);
		if	(	nextButOne != &ctx.agent
			||	nextButOneDist > ctx.configuration.GapBetweenAgents
			)
		{
			float otc = ctx.local_ai_ctrl.isOppositeTrackClear(*nextAgent, distanceToNextAgent, speedDiff, curSpeed, true);
			abort = otc < 1.0f;
		}
		else
			abort = nextButOne == &ctx.agent;
	}

	return abort;
}
