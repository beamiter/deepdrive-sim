
#include "DeepDrivePluginPrivatePCH.h"
#include "Private/Simulation/Agent/Controllers/LocalAI/States/DeepDriveAgentPassingState.h"
#include "Private/Simulation/Agent/Controllers/DeepDriveAgentSpeedController.h"
#include "Private/Simulation/Agent/Controllers/DeepDriveAgentSteeringController.h"
#include "Public/Simulation/Agent/Controllers/DeepDriveAgentLocalAIController.h"
#include "Public/Simulation/Agent/DeepDriveAgent.h"


DeepDriveAgentPassingState::DeepDriveAgentPassingState(DeepDriveAgentLocalAIStateMachine &stateMachine)
	: DeepDriveAgentLocalAIStateBase(stateMachine, "Passing")
{

}


void DeepDriveAgentPassingState::enter(DeepDriveAgentLocalAIStateMachineContext &ctx)
{
	startThinkTimer(ctx.configuration.ThinkDelays.Z, false);
	m_AgentToPass = ctx.agent.getNextAgent(-1.0f);
	
	UE_LOG(LogDeepDriveAgentLocalAIController, Log, TEXT(">>>> Passing agent %d/%s"), m_AgentToPass ? m_AgentToPass->getAgentId() : -1, *(m_AgentToPass ? *(m_AgentToPass->GetName()) : FString("")) );

	ctx.local_ai_ctrl.setIsPassing(true);
}

void DeepDriveAgentPassingState::update(DeepDriveAgentLocalAIStateMachineContext &ctx, float dT)
{
	if	(	m_AgentToPass
		&&	ctx.agent.getNextAgent(-1.0f) != &ctx.agent
		)
	{
		const float passedDistance = ctx.local_ai_ctrl.getPassedDistance(m_AgentToPass);
		if(passedDistance > 0.0f)
			m_StateMachine.setNextState("PullIn");
		else if(isTimeToThink(dT))
		{
			if(abortOvertaking(ctx))
				m_StateMachine.setNextState("AbortOvertaking");
		}
	}
	else
	{
		m_StateMachine.setNextState("PullIn");
	}


	float desiredSpeed = ctx.configuration.OvertakingSpeed;
	desiredSpeed = ctx.speed_controller.limitSpeedByTrack(desiredSpeed, ctx.configuration.SpeedLimitFactor);

	// ctx.speed_controller.update(dT, desiredSpeed);
	ctx.speed_controller.update(dT, desiredSpeed, -1.0f, 0.0f);
	ctx.steering_controller.update(dT, desiredSpeed, ctx.side_offset);
}

void DeepDriveAgentPassingState::exit(DeepDriveAgentLocalAIStateMachineContext &ctx)
{
	ctx.local_ai_ctrl.setIsPassing(false);
}

bool DeepDriveAgentPassingState::abortOvertaking(DeepDriveAgentLocalAIStateMachineContext &ctx)
{
	bool abort = false;

	const float curSpeed = ctx.agent.getSpeedKmh();

	float otc = ctx.local_ai_ctrl.computeOppositeTrackClearance(curSpeed, ctx.configuration.LookAheadTime);
	abort = otc < 0.10f;

	if(abort)
		UE_LOG(LogDeepDriveAgentLocalAIController, Log, TEXT("Abort overtaking %s / %d  otc %f"), *(m_AgentToPass ? *(m_AgentToPass->GetName()) : FString("---")), m_AgentToPass ? m_AgentToPass->getAgentId() : -1, otc );


	return abort;
}
