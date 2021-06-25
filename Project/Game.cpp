#include "Game.h"
#include "Utilities.h"
#include <iostream>

Game::Game() 
	: m_running(false)
	, m_pWindow(nullptr)
	, m_pRenderer(nullptr)
	, m_keyStates(nullptr)
	, m_shouldClearEnemies(false)
	, m_shouldClearPlayerBullets(false)
	, m_shouldClearEnemyBullets(false)
	, m_canShoot(true)
	, m_pBGText(nullptr)
	, m_pSpriteTexture(nullptr)
	, m_pPlayer(nullptr)
	, m_enemySpawnTime(0)
	, m_enemySpawnMaxTime(1.5f)
	, m_pivot({ 0,0 })
	, m_pMusic(nullptr)
{

}

Game::~Game()
{

}

Game* Game::GetInstance()
{
	if (s_instance == nullptr)
	{
		s_instance = new Game();
	}
	return s_instance;
}

void Game::DeleteInstance()
{
	delete s_instance;
	s_instance = nullptr;
}


int Game::Init(const char* title, int xPos, int yPos)
{
	std::cout << "Init Game ..." << std::endl;
	srand((unsigned)time(nullptr));

	int errorCode = SDL_Init(SDL_INIT_EVERYTHING);
	if (errorCode == 0)
	{
		std::cout << "SDL_Init() succeeded!" << std::endl;
	}
	else
	{
		std::cout << "SDL_Init() failed. Error code: " << errorCode << ": " << SDL_GetError() << std::endl;
		system("pause");
		return errorCode;
	}

	m_pWindow = SDL_CreateWindow(title, xPos, yPos, kWidth, kHeight, 0);
	if (m_pWindow != nullptr)
	{
		std::cout << "SDL_CreateWindow() succeeded!" << std::endl;
	}
	else
	{
		std::cout << "SDL_CreateWindow() failed. Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		system("pause");
		return -1;
	}

	m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, SDL_RENDERER_ACCELERATED);
	if (m_pRenderer != nullptr)
	{
		std::cout << "SDL_CreateRenderer() succeeded!" << std::endl;
	}
	else
	{
		std::cout << "SDL_CreateRenderer() failed. Error: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(m_pWindow);
		SDL_Quit();
		system("pause");
		return -1;
	}

	errorCode = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 1024);
	if (errorCode == 0)
	{
		std::cout << "Mix_OpenAudio() succeeded!" << std::endl;
	}
	else
	{
		std::cout << "Mix_OpenAudio() failed. Error: " << Mix_GetError() << std::endl;
		SDL_DestroyWindow(m_pWindow);
		SDL_Quit();
		system("pause");
		return -1;
	}

	int audioFlags = MIX_INIT_MP3;
	errorCode = Mix_Init(audioFlags);
	if (errorCode != audioFlags)
	{
		std::cout << "Mix_OpenAudio() failed. Error: " << Mix_GetError() << std::endl;
		SDL_DestroyWindow(m_pWindow);
		SDL_Quit();
		system("pause");
		return -1;
	}
	std::cout << "Initialization successful!" << std::endl;

	m_keyStates = SDL_GetKeyboardState(nullptr);

	m_pBGText = IMG_LoadTexture(m_pRenderer, "Assets/Images/background.png");
	m_pSpriteTexture = IMG_LoadTexture(m_pRenderer, "Assets/Images/sprites.png");

	m_bgArray[0] = { {0, 0, kWidth, kHeight}, {0, 0, kWidth, kHeight} };
	m_bgArray[1] = { {0, 0, kWidth, kHeight}, {kWidth, 0, kWidth, kHeight} };

	m_pMusic = Mix_LoadMUS("Assets/Audio/game.mp3"); // Load the music track.
	if (m_pMusic == nullptr)
	{
		std::cout << "Failed to load music: " << Mix_GetError() << std::endl;
	}

	m_sounds.reserve(3);
	Mix_Chunk* sound = Mix_LoadWAV("Assets/Audio/enemy.wav");
	if (sound == nullptr)
	{
		std::cout << "Failed to load sound: " << Mix_GetError() << std::endl;
	}
	m_sounds.push_back(sound);

	sound = Mix_LoadWAV("Assets/Audio/laser.wav");
	if (sound == nullptr)
	{
		std::cout << "Failed to load sound: " << Mix_GetError() << std::endl;
	}
	m_sounds.push_back(sound);

	sound = Mix_LoadWAV("Assets/Audio/explode.wav");
	if (sound == nullptr)
	{
		std::cout << "Failed to load sound: " << Mix_GetError() << std::endl;
	}
	m_sounds.push_back(sound);

	m_pPlayer = new Player( { kPlayerSourceXPosition, kPlayerSourceYPosition, kPlayerWidth, kPlayerHeight }, // Image Source
							{ kWidth / 4 ,kHeight / 2 - kPlayerHeight / 2,kPlayerWidth,kPlayerHeight } ); // Window Destination

	Mix_PlayMusic(m_pMusic, -1); // Play. -1 = looping.
	Mix_VolumeMusic(16); // 0-MIX_MAX_VOLUME (128).

	m_running = true;
	return 0;
}

bool Game::IsRunning()
{
	return m_running;
}

void Game::HandleEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_QUIT: // User pressed window's 'x' button.
			QuitGame();
			break;
		case SDL_KEYDOWN: // Try SDL_KEYUP instead.
			if (event.key.keysym.sym == SDLK_ESCAPE)
				QuitGame();
			break;
		case SDL_KEYUP:
			if (event.key.keysym.sym == SDLK_SPACE)
				m_canShoot = true;
			break;
		}
	}
}

bool Game::KeyDown(SDL_Scancode key)
{
	if(m_keyStates)
	{
		return m_keyStates[key] == 1;
	}
	return false;
}



/* Update is SUPER way too long on purpose! Part of the Assignment 1, if you use
   this program as a start project is to chop up Update and figure out where each
   part of the code is supposed to go. A practice in OOP is to have objects handle
   their own behaviour and this is a big hint for you. */
void Game::Update(float deltaTime)
{
	// Scroll the backgrounds. 
	for (int i = 0; i < kNumberOfBackgrounds; i++)
	{
		m_bgArray[i].GetDestinationTransform()->x -= kBackgroundScrollSpeed * deltaTime;
	}
	
	// Check if they need to snap back.
	if (m_bgArray[1].GetDestinationTransform()->x <= 0)
	{
		m_bgArray[0].GetDestinationTransform()->x = 0;
		m_bgArray[1].GetDestinationTransform()->x = kWidth;
	}
	
	// Player animation/movement.
	m_pPlayer->Animate(deltaTime); // Oh! We're telling the player to animate itself. This is good! Hint hint.
	
	if (KeyDown(SDL_SCANCODE_A) && m_pPlayer->GetDestinationTransform()->x > m_pPlayer->GetDestinationTransform()->h)
	{
		m_pPlayer->GetDestinationTransform()->x -= kPlayerSpeed * deltaTime;
	}
	else if (KeyDown(SDL_SCANCODE_D) && m_pPlayer->GetDestinationTransform()->x < kWidth / 2)
	{
		m_pPlayer->GetDestinationTransform()->x += kPlayerSpeed * deltaTime;
	}
	if (KeyDown(SDL_SCANCODE_W) && m_pPlayer->GetDestinationTransform()->y > 0)
	{
		m_pPlayer->GetDestinationTransform()->y -= kPlayerSpeed * deltaTime;
	}
	else if (KeyDown(SDL_SCANCODE_S) && m_pPlayer->GetDestinationTransform()->y < kHeight - m_pPlayer->GetDestinationTransform()->w)
	{
		m_pPlayer->GetDestinationTransform()->y += kPlayerSpeed * deltaTime;
	}
	
	if (KeyDown(SDL_SCANCODE_SPACE) && m_canShoot)
	{
		m_canShoot = false;
		m_playerBullets.push_back(
			new Bullet(	{ kPlayerBulletSourceXPosition, kPlayerBulletSourceYPosition, kPlayerBulletWidth, kPlayerBulletHeight },
						{ m_pPlayer->GetDestinationTransform()->x + kPlayerWidth - kPlayerBulletWidth, m_pPlayer->GetDestinationTransform()->y + kPlayerHeight / 2 - kPlayerBulletWidth, kPlayerBulletWidth, kPlayerBulletHeight },
						kPlayerBulletSpeed));
		Mix_PlayChannel(-1, m_sounds[1], 0);
	}

	// Enemy animation/movement.
	for (size_t i = 0; i < m_enemies.size(); i++)
	{
		m_enemies[i]->Update(deltaTime);
		if (m_enemies[i]->GetDestinationTransform()->x < -m_enemies[i]->GetDestinationTransform()->h)
		{
			delete m_enemies[i];
			m_enemies[i] = nullptr;
			m_shouldClearEnemies = true;
		}
	}
	if (m_shouldClearEnemies)
	{
		CleanVector<Enemy*>(m_enemies, m_shouldClearEnemies);
	}

	// Update enemy spawns.
	m_enemySpawnTime += deltaTime;
	if (m_enemySpawnTime > m_enemySpawnMaxTime)
	{
		// Randomizing enemy bullet spawn rate
		float bulletSpawnRate = 0.5f + (rand() % 3) / 2.0f;
		// Random starting y location
		float yEnemyLocation = (float)(kEnemyHeight + rand() % (kHeight - kEnemyHeight));
		m_enemies.push_back(
			new Enemy( { kEnemySourceXPosition, kEnemySourceYPosition, kEnemyWidth, kEnemyHeight },
					   { kWidth, yEnemyLocation, kEnemyWidth, kEnemyHeight },
					   &m_enemyBullets,
					   m_sounds[0],
					   bulletSpawnRate)); 

		m_enemySpawnTime = 0;
	}

	// Update the bullets. Player's first.
	for (size_t i = 0; i < m_playerBullets.size(); i++)
	{
		m_playerBullets[i]->Update(deltaTime);
		if (m_playerBullets[i]->GetDestinationTransform()->x > kWidth)
		{
			delete m_playerBullets[i];
			m_playerBullets[i] = nullptr;
			m_shouldClearPlayerBullets = true;
		}
	}

	if (m_shouldClearPlayerBullets)
	{
		CleanVector<Bullet*>(m_playerBullets, m_shouldClearPlayerBullets);
	}

	// Now enemy bullets.
	for (size_t i = 0; i < m_enemyBullets.size(); i++)
	{
		m_enemyBullets[i]->Update(deltaTime);
		if (m_enemyBullets[i]->GetDestinationTransform()->x < -m_enemyBullets[i]->GetDestinationTransform()->w)
		{
			delete m_enemyBullets[i];
			m_enemyBullets[i] = nullptr;
			m_shouldClearEnemyBullets = true;
		}
	}

	if (m_shouldClearEnemyBullets)
	{
		CleanVector<Bullet*>(m_enemyBullets, m_shouldClearEnemyBullets);
	}

	CheckCollision();
}

void Game::Render()
{
	SDL_SetRenderDrawColor(m_pRenderer, 0, 0, 0, 255);
	SDL_RenderClear(m_pRenderer); // Clear the screen with the draw color.
	
	// Render stuff. Background first.
	for (int i = 0; i < kNumberOfBackgrounds; i++)
	{
		SDL_RenderCopyF(m_pRenderer, m_pBGText, m_bgArray[i].GetSourceTransform(), m_bgArray[i].GetDestinationTransform());
	}

	// Player.
	SDL_RenderCopyExF(	m_pRenderer, 
						m_pSpriteTexture, 
						m_pPlayer->GetSourceTransform(), 
						m_pPlayer->GetDestinationTransform(), 
						m_pPlayer->GetAngle(), 
						&m_pivot, 
						SDL_FLIP_NONE);
	
	// Player bullets.	
	for (size_t i = 0; i < m_playerBullets.size(); i++)
	{
		SDL_RenderCopyExF(m_pRenderer, m_pSpriteTexture, m_playerBullets[i]->GetSourceTransform(), m_playerBullets[i]->GetDestinationTransform(), 90, &m_pivot, SDL_FLIP_NONE);
	}
	
	// Enemies.
	for (size_t i = 0; i < m_enemies.size(); i++)
	{
		SDL_RenderCopyExF(m_pRenderer, m_pSpriteTexture, m_enemies[i]->GetSourceTransform(), m_enemies[i]->GetDestinationTransform(), -90, &m_pivot, SDL_FLIP_NONE);
	}
	
	// Enemy bullets.
	for (size_t i = 0; i < m_enemyBullets.size(); i++)
	{
		SDL_RenderCopyF(m_pRenderer, m_pSpriteTexture, m_enemyBullets[i]->GetSourceTransform(), m_enemyBullets[i]->GetDestinationTransform());
	}

	SDL_RenderPresent(m_pRenderer);
}

void Game::Clean()
{
	std::cout << "Clean ..." << std::endl;

	// Delete Player
	delete m_pPlayer;
	m_pPlayer = nullptr;

	// Destroy Enemies
	for (size_t i = 0; i < m_enemies.size(); i++)
	{
		delete m_enemies[i];
		m_enemies[i] = nullptr;
	}
	m_enemies.clear();

	// Destroy Player Bullets
	for (size_t i = 0; i < m_playerBullets.size(); i++)
	{
		delete m_playerBullets[i];
		m_playerBullets[i] = nullptr;
	}
	m_playerBullets.clear();

	// Destroy Enemy Bullets
	for (size_t i = 0; i < m_enemyBullets.size(); i++)
	{
		delete m_enemyBullets[i];
		m_enemyBullets[i] = nullptr;
	}
	m_enemyBullets.clear();

	// Clean sounds up
	for (size_t i = 0; i < m_sounds.size(); i++)
	{
		Mix_FreeChunk(m_sounds[i]);
	}
	m_sounds.clear();

	Mix_FreeMusic(m_pMusic);
	Mix_CloseAudio();
	Mix_Quit();
	
	SDL_DestroyRenderer(m_pRenderer);
	SDL_DestroyWindow(m_pWindow);
	SDL_Quit();
}

void Game::CheckCollision()
{
	// Player vs. Enemy.
	SDL_Rect playerCollisionRect = { (int)m_pPlayer->GetDestinationTransform()->x - kPlayerHeight, (int)m_pPlayer->GetDestinationTransform()->y, kPlayerHeight, kPlayerWidth };
	for (size_t i = 0; i < m_enemies.size(); i++)
	{
		SDL_Rect enemyCollisionRect = { (int)m_enemies[i]->GetDestinationTransform()->x, (int)m_enemies[i]->GetDestinationTransform()->y - kEnemyWidth, kEnemyHeight, kEnemyWidth };
		if (SDL_HasIntersection(&playerCollisionRect, &enemyCollisionRect))
		{
			// Game over!
			std::cout << "Player goes boom!" << std::endl;
			Mix_PlayChannel(-1, m_sounds[2], 0);
			delete m_enemies[i];
			m_enemies[i] = nullptr;
			m_shouldClearEnemies = true;
		}
	}

	if (m_shouldClearEnemies)
	{
		CleanVector<Enemy*>(m_enemies, m_shouldClearEnemies);
	}

	// Player bullets vs. Enemies.
	for (size_t i = 0; i < m_playerBullets.size(); i++)
	{
		SDL_Rect bulletCollisionRect = { (int)m_playerBullets[i]->GetDestinationTransform()->x - kPlayerBulletHeight, (int)m_playerBullets[i]->GetDestinationTransform()->y, kPlayerBulletHeight, kPlayerBulletWidth };
		for (size_t j = 0; j < m_enemies.size(); j++)
		{
			if (m_enemies[j] != nullptr)
			{
				SDL_Rect enemyCollisionRect = { (int)m_enemies[j]->GetDestinationTransform()->x, (int)m_enemies[j]->GetDestinationTransform()->y - kEnemyWidth, kEnemyHeight, kEnemyWidth };
				if (SDL_HasIntersection(&bulletCollisionRect, &enemyCollisionRect))
				{
					Mix_PlayChannel(-1, m_sounds[2], 0);
					delete m_enemies[j];
					m_enemies[j] = nullptr;
					delete m_playerBullets[i];
					m_playerBullets[i] = nullptr;
					m_shouldClearEnemies = true;
					m_shouldClearPlayerBullets = true;
					break;
				}
			}
		}
	}
	
	if (m_shouldClearEnemies)
	{
		CleanVector<Enemy*>(m_enemies, m_shouldClearEnemies);
	}

	if (m_shouldClearPlayerBullets)
	{
		CleanVector<Bullet*>(m_playerBullets, m_shouldClearPlayerBullets);
	}

	// Enemy bullets vs. player.
	for (size_t i = 0; i < m_enemyBullets.size(); i++)
	{
		SDL_Rect enemyBulletCollisionRect = { (int)m_enemyBullets[i]->GetDestinationTransform()->x, (int)m_enemyBullets[i]->GetDestinationTransform()->y, (int)m_enemyBullets[i]->GetDestinationTransform()->w, (int)m_enemyBullets[i]->GetDestinationTransform()->h };
		if (SDL_HasIntersection(&playerCollisionRect, &enemyBulletCollisionRect))
		{
			// Game over!
			std::cout << "Player goes boom!" << std::endl;
			Mix_PlayChannel(-1, m_sounds[2], 0);
			delete m_enemyBullets[i];
			m_enemyBullets[i] = nullptr;
			m_shouldClearEnemyBullets = true;
			break;
		}
	}

	if (m_shouldClearEnemyBullets)
	{
		CleanVector<Bullet*>(m_enemyBullets, m_shouldClearEnemyBullets);
	}
}