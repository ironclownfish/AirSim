#include "ManualPoseController.h"
#include "AirBlueprintLib.h"


void UManualPoseController::initializeForPlay()
{
    actor_ = nullptr;
    clearBindings();

    left_mapping_		= FInputAxisKeyMapping("inputManualArrowLeft",	EKeys::A);
	right_mapping_		= FInputAxisKeyMapping("inputManualArrowRight", EKeys::D); 
    forward_mapping_	= FInputAxisKeyMapping("inputManualForward",	EKeys::W);
	backward_mapping_	= FInputAxisKeyMapping("inputManualBackward",	EKeys::S);
    up_mapping_			= FInputAxisKeyMapping("inputManualArrowUp",	EKeys::E);
	down_mapping_		= FInputAxisKeyMapping("inputManualArrowDown",	EKeys::Q); 
    yaw_mapping_		= FInputAxisKeyMapping("inputManualYaw",		EKeys::MouseX);
    pitch_mapping_		= FInputAxisKeyMapping("inputManualPitch",		EKeys::MouseY);

    input_positive_ = inpute_negative_ = last_velocity_ = FVector::ZeroVector;
}

void UManualPoseController::clearBindings()
{
    left_binding_ = right_binding_ = up_binding_ = down_binding_ = nullptr;
    forward_binding_ = backward_binding_ = left_yaw_binding_ = up_pitch_binding_ = nullptr;
    right_yaw_binding_ = down_pitch_binding_ = nullptr;
}

void UManualPoseController::setActor(AActor* actor)
{
    //if we already have attached actor
    if (actor_) {
        removeInputBindings();
    }

    actor_ = actor;

    if (actor_ != nullptr) {
        resetDelta();
        setupInputBindings();
    }
}

AActor* UManualPoseController::getActor() const
{
    return actor_;
}

void UManualPoseController::updateActorPose(float dt)
{
    if (getActor() != nullptr) {
        updateDeltaPosition(dt);

        FVector location = getActor()->GetActorLocation();
        FRotator rotation = getActor()->GetActorRotation();
		getActor()->SetActorLocationAndRotation(location + delta_position_, rotation + delta_rotation_);
        resetDelta();
    }
    else {
        UAirBlueprintLib::LogMessageString("UManualPoseController::updateActorPose should not be called when actor is not set", "", LogDebugLevel::Failure);
    }
}

void UManualPoseController::getDeltaPose(FVector& delta_position, FRotator& delta_rotation) const
{
    delta_position = delta_position_;
    delta_rotation = delta_rotation_;
}

void UManualPoseController::resetDelta()
{
    delta_position_ = FVector::ZeroVector;
    delta_rotation_ = FRotator::ZeroRotator;
}

void UManualPoseController::removeInputBindings()
{
    if (left_binding_)
        UAirBlueprintLib::RemoveAxisBinding(left_mapping_, left_binding_, getActor());
    if (right_binding_)
        UAirBlueprintLib::RemoveAxisBinding(right_mapping_, right_binding_, getActor());
    if (forward_binding_)
        UAirBlueprintLib::RemoveAxisBinding(forward_mapping_, forward_binding_, getActor());
    if (backward_binding_)
        UAirBlueprintLib::RemoveAxisBinding(backward_mapping_, backward_binding_, getActor());
    if (up_binding_)
        UAirBlueprintLib::RemoveAxisBinding(up_mapping_, up_binding_, getActor());
    if (down_binding_)
        UAirBlueprintLib::RemoveAxisBinding(down_mapping_, down_binding_, getActor());
    if (left_yaw_binding_)
        UAirBlueprintLib::RemoveAxisBinding(left_yaw_mapping_, left_yaw_binding_, getActor());
    if (up_pitch_binding_)
        UAirBlueprintLib::RemoveAxisBinding(up_pitch_mapping_, up_pitch_binding_, getActor());
    if (right_yaw_binding_)
        UAirBlueprintLib::RemoveAxisBinding(right_yaw_mapping_, right_yaw_binding_, getActor());
    if (down_pitch_binding_)
        UAirBlueprintLib::RemoveAxisBinding(down_pitch_mapping_, down_pitch_binding_, getActor());
	if (yaw_binding_)
		UAirBlueprintLib::RemoveAxisBinding(yaw_mapping_, yaw_binding_, getActor());
	if (pitch_binding_)
		UAirBlueprintLib::RemoveAxisBinding(pitch_mapping_, pitch_binding_, getActor());


    clearBindings();
}

void UManualPoseController::setupInputBindings()
{
    UAirBlueprintLib::EnableInput(getActor());

    left_binding_ = & UAirBlueprintLib::BindAxisToKey(left_mapping_, getActor(), this, &UManualPoseController::inputManualLeft);
    right_binding_ = & UAirBlueprintLib::BindAxisToKey(right_mapping_, getActor(), this, &UManualPoseController::inputManualRight);
    forward_binding_ = & UAirBlueprintLib::BindAxisToKey(forward_mapping_, getActor(), this, &UManualPoseController::inputManualForward);
    backward_binding_ = & UAirBlueprintLib::BindAxisToKey(backward_mapping_, getActor(), this, &UManualPoseController::inputManualBackward);
    up_binding_ = & UAirBlueprintLib::BindAxisToKey(up_mapping_, getActor(), this, &UManualPoseController::inputManualMoveUp);
    down_binding_ = & UAirBlueprintLib::BindAxisToKey(down_mapping_, getActor(), this, &UManualPoseController::inputManualDown);

	// Set up "look" bindings.
	yaw_binding_ = &UAirBlueprintLib::BindAxisToKey(yaw_mapping_, getActor(), this, &UManualPoseController::inputManualRightYaw);
	pitch_binding_ = &UAirBlueprintLib::BindAxisToKey(pitch_mapping_, getActor(), this, &UManualPoseController::inputManualUpPitch);

	setMouseLookMode(true);
}

void UManualPoseController::setMouseLookMode(bool on)
{
	if (on)
		getActor()->GetWorld()->GetFirstPlayerController()->SetInputMode(FInputModeGameAndUI());
}

bool UManualPoseController::isShiftKeyDown()
{
	return getActor()->GetWorld()->GetFirstPlayerController()->IsInputKeyDown(EKeys::LeftShift);
}

float UManualPoseController::speedMultiplier()
{
	return isShiftKeyDown() ? shift_key_speed_multiplier_ : 1.0;
}

void UManualPoseController::updateDeltaPosition(float dt)
{
    FVector input = input_positive_ - inpute_negative_;
    if (!FMath::IsNearlyZero(input.SizeSquared())) {
        if (FMath::IsNearlyZero(acceleration_))
            last_velocity_ = input * 1000;
        else
            last_velocity_ += input * (acceleration_ * dt);
        delta_position_ += getActor()->GetActorRotation().RotateVector(last_velocity_ * dt);
    } else {
        delta_position_ = last_velocity_ = FVector::ZeroVector;
    }
}

void UManualPoseController::inputManualLeft(float val)
{
    inpute_negative_.Y = speedMultiplier() * val;
}
void UManualPoseController::inputManualRight(float val)
{
    input_positive_.Y = speedMultiplier() * val;
}
void UManualPoseController::inputManualForward(float val)
{
    input_positive_.X = speedMultiplier() * val;
}
void UManualPoseController::inputManualBackward(float val)
{
    inpute_negative_.X = speedMultiplier() * val;
}
void UManualPoseController::inputManualMoveUp(float val)
{
    input_positive_.Z = speedMultiplier() * val;
}
void UManualPoseController::inputManualDown(float val)
{
    inpute_negative_.Z = speedMultiplier() * val;
}
void UManualPoseController::inputManualLeftYaw(float val)
{
    if (!FMath::IsNearlyEqual(val, 0.f))
        delta_rotation_.Add(0, -val, 0);
}
void UManualPoseController::inputManualUpPitch(float val)
{
    if (!FMath::IsNearlyEqual(val, 0.f))
        delta_rotation_.Add(val, 0, 0);
}
void UManualPoseController::inputManualRightYaw(float val)
{
    if (!FMath::IsNearlyEqual(val, 0.f))
        delta_rotation_.Add(0, val, 0);
}
void UManualPoseController::inputManualDownPitch(float val)
{
    if (!FMath::IsNearlyEqual(val, 0.f))
        delta_rotation_.Add(-val, 0, 0);
}
