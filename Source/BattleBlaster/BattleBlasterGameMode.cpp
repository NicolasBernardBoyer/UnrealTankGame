// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleBlasterGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "Tower.h"
#include "BattleBlasterGameInstance.h"

void ABattleBlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> Towers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATower::StaticClass(), Towers);
	TowerCount = Towers.Num();
	UE_LOG(LogTemp, Display, TEXT("Number of towers: %d"), TowerCount);

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (PlayerPawn) {
		Tank = Cast<ATank>(PlayerPawn);
		if (!Tank) {
			UE_LOG(LogTemp, Display, TEXT("GameMode: Failed to find tank actor!"));
		}
	}

	int32 loopIndex = 0;
	while (loopIndex < TowerCount) {
		AActor* TowerActor = Towers[loopIndex];
		if (TowerActor) {
			ATower* Tower = Cast<ATower>(TowerActor);
			if (Tower && Tank) {
				Tower->Tank = Tank;
				UE_LOG(LogTemp, Display, TEXT("%s setting the tank variable!"), *Tower->GetActorNameOrLabel());
			}
		}

		loopIndex++;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController) {
		ScreenMessageWidget = CreateWidget<UScreenMessage>(PlayerController, ScreenMessageClass);
		if (ScreenMessageWidget) {
			ScreenMessageWidget->AddToPlayerScreen();
			ScreenMessageWidget->SetMessageText("GET READY!");
		}
	}


	CountdownSeconds = CountdownDelay;
	GetWorldTimerManager().SetTimer(CountdownTimerHandle, this, &ABattleBlasterGameMode::OnCountdownTimerTimeout, 1.0f, true);
}

void ABattleBlasterGameMode::OnCountdownTimerTimeout()
{
	CountdownSeconds--;

	if (CountdownSeconds > 0) {
		ScreenMessageWidget->SetMessageText(FString::FromInt(CountdownSeconds));
	}
	else if (CountdownSeconds == 0) {
		ScreenMessageWidget->SetMessageText("GO!");
		Tank->SetPlayerEnabled(true);
	}
	else {
		// Countdown is over, so enable the player tank and stop the timer
		GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
		ScreenMessageWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}


void ABattleBlasterGameMode::ActorDied(AActor* DeadActor)
{
	bool IsGameOver = false;

	if (DeadActor == Tank) {
		// Tank just died
		IsGameOver = true;
		Tank->HandleDestruction();
	}
	else
	{
		// A Tower just died
		ATower* DeadTower = Cast<ATower>(DeadActor);
		if (DeadTower) {
			DeadTower->HandleDestruction();

			TowerCount--;
			if (TowerCount == 0) {
				IsVictory = true;
				IsGameOver = true;
			}
		}
	}

	if (IsGameOver) {
		FString GameOverString = IsVictory ? "VICTORY!" : "DEFEAT!";

		if (ScreenMessageWidget) {
			ScreenMessageWidget->SetMessageText(GameOverString);
			ScreenMessageWidget->SetVisibility(ESlateVisibility::Visible);
		}

		FTimerHandle GameOverTimerHandle;
		GetWorldTimerManager().SetTimer(GameOverTimerHandle, this, &ABattleBlasterGameMode::OnGameOverTimerTimeout, GameOverDelay, false);
	}
}

void ABattleBlasterGameMode::OnGameOverTimerTimeout()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance) {
		UBattleBlasterGameInstance* BattleBlasterGameInstance = Cast<UBattleBlasterGameInstance>(GameInstance);
		if (BattleBlasterGameInstance) {
			if (IsVictory) {
				BattleBlasterGameInstance->LoadNextLevel();
			}
			else {
				BattleBlasterGameInstance->RestartCurrentLevel();
			}
		}
	}
}