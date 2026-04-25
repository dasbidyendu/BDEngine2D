local TILE_SIZE = 80
local PIECE_SIZE = 64
local BOARD_START_X = 200
local BOARD_START_Y = 100

local boardInitialized = false
local grid = {}    
local pieces = {}  
local highlightPool = {} 
local selectHighlight = -1

local GameState = {
    turn = "white",
    selectedPiece = nil,
    validMoves = {},
    validMovesCount = 0,
    isGameOver = false
}

local COLOR_WHITE  = MakeColor(255, 255, 255, 255)
local COLOR_GRAY   = MakeColor(120, 120, 120, 255)
local COLOR_SELECT = MakeColor(255, 255, 0, 160)
local COLOR_VALID  = MakeColor(0, 255, 0, 160)

local pieceOrder = {"rook", "knight", "bishop", "queen", "king", "bishop", "knight", "rook"}

-----------------------------------------------------------
-- PATHFINDING & VALIDATION
-----------------------------------------------------------
local function IsPathClear(fx, fy, tx, ty)
    local stepX = (tx == fx) and 0 or (tx > fx and 1 or -1)
    local stepY = (ty == fy) and 0 or (ty > fy and 1 or -1)
    local currX, currY = fx + stepX, fy + stepY
    while currX ~= tx or currY ~= ty do
        if grid[currY][currX] ~= nil then return false end
        currX, currY = currX + stepX, currY + stepY
    end
    return true
end

local Validator = {}
Validator["pawn"] = function(fx, fy, tx, ty, team, isCapture)
    local dir = (team == "black") and 1 or -1
    local startRow = (team == "black") and 1 or 6
    local dx, dy = tx - fx, ty - fy
    if not isCapture and dx == 0 then
        if dy == dir then return (grid[ty][tx] == nil) end
        if fy == startRow and dy == dir * 2 then
            return grid[fy + dir][fx] == nil and grid[ty][tx] == nil
        end
    elseif isCapture and math.abs(dx) == 1 and dy == dir then
        return true
    end
    return false
end

Validator["rook"] = function(fx, fy, tx, ty)
    if fx ~= tx and fy ~= ty then return false end
    return IsPathClear(fx, fy, tx, ty)
end

Validator["bishop"] = function(fx, fy, tx, ty)
    if math.abs(tx - fx) ~= math.abs(ty - fy) then return false end
    return IsPathClear(fx, fy, tx, ty)
end

Validator["knight"] = function(fx, fy, tx, ty)
    local dx, dy = math.abs(tx - fx), math.abs(ty - fy)
    return (dx == 2 and dy == 1) or (dx == 1 and dy == 2)
end

Validator["queen"] = function(fx, fy, tx, ty)
    local straight = (fx == tx or fy == ty)
    local diag = (math.abs(tx - fx) == math.abs(ty - fy))
    return (straight or diag) and IsPathClear(fx, fy, tx, ty)
end

Validator["king"] = function(fx, fy, tx, ty)
    return math.abs(tx - fx) <= 1 and math.abs(ty - fy) <= 1
end

-----------------------------------------------------------
-- CHECK & CHECKMATE LOGIC
-----------------------------------------------------------
function GetKingPos(team)
    for id, p in pairs(pieces) do
        if p.type == "king" and p.team == team then return p.gx, p.gy end
    end
    return -1, -1
end

function IsSquareAttacked(tx, ty, attackerTeam)
    for id, p in pairs(pieces) do
        if p.team == attackerTeam then
            if Validator[p.type](p.gx, p.gy, tx, ty, p.team, true) then return true end
        end
    end
    return false
end

function WouldBeInCheck(fx, fy, tx, ty, team)
    local movingPiece = grid[fy][fx]
    local targetPiece = grid[ty][tx]
    local enemyTeam = (team == "white") and "black" or "white"
    
    local oldX, oldY = pieces[movingPiece].gx, pieces[movingPiece].gy
    local tempMeta = nil
    if targetPiece then tempMeta = pieces[targetPiece] pieces[targetPiece] = nil end

    grid[ty][tx] = movingPiece
    grid[fy][fx] = nil
    pieces[movingPiece].gx, pieces[movingPiece].gy = tx, ty

    local kx, ky = GetKingPos(team)
    local inCheck = IsSquareAttacked(kx, ky, enemyTeam)

    grid[fy][fx] = movingPiece
    grid[ty][tx] = targetPiece
    pieces[movingPiece].gx, pieces[movingPiece].gy = oldX, oldY
    if targetPiece then pieces[targetPiece] = tempMeta end

    return inCheck
end

function CheckCheckmate(team)
    for id, p in pairs(pieces) do
        if p.team == team then
            for y = 0, 7 do
                for x = 0, 7 do
                    if not (x == p.gx and y == p.gy) then
                        local target = grid[y][x]
                        local isFriendly = target and (pieces[target].team == p.team)
                        if not isFriendly and Validator[p.type](p.gx, p.gy, x, y, p.team, target ~= nil) then
                            if not WouldBeInCheck(p.gx, p.gy, x, y, p.team) then return false end
                        end
                    end
                end
            end
        end
    end
    local kx, ky = GetKingPos(team)
    local enemy = (team == "white") and "black" or "white"
    if IsSquareAttacked(kx, ky, enemy) then print("CHECKMATE! " .. enemy .. " wins")
    else print("STALEMATE!") end
    GameState.isGameOver = true
    return true
end

-----------------------------------------------------------
-- INTERACTION
-----------------------------------------------------------
function ShowValidMoves(entity)
    local data = pieces[entity]
    GameState.validMoves = {}
    GameState.validMovesCount = 0
    local poolIdx = 1
    for i=1, 28 do SetPos(highlightPool[i], -1000, -1000) end

    for y = 0, 7 do
        for x = 0, 7 do
            if not (x == data.gx and y == data.gy) then
                local target = grid[y][x]
                local isFriendly = target and (pieces[target].team == data.team)
                if not isFriendly and Validator[data.type](data.gx, data.gy, x, y, data.team, target ~= nil) then
                    if not WouldBeInCheck(data.gx, data.gy, x, y, data.team) then
                        GameState.validMovesCount = GameState.validMovesCount + 1
                        GameState.validMoves[GameState.validMovesCount] = {x = x, y = y}
                        if highlightPool[poolIdx] then
                            SetPos(highlightPool[poolIdx], BOARD_START_X + (x * TILE_SIZE), BOARD_START_Y + (y * TILE_SIZE))
                            poolIdx = poolIdx + 1
                        end
                    end
                end
            end
        end
    end
end

function HandleInput(mx, my)
    if GameState.isGameOver then return end
    local worldPos = ScreenToWorld(mx, my)
    local wx, wy = worldPos.x, worldPos.y

    -- Use wx and wy instead of mx and my for grid detection
    local gx = math.floor((wx - BOARD_START_X + (TILE_SIZE / 2)) / TILE_SIZE)
    local gy = math.floor((wy - BOARD_START_Y + (TILE_SIZE / 2)) / TILE_SIZE)
    
    if gx < 0 or gx > 7 or gy < 0 or gy > 7 then return end

    local clickedPiece = grid[gy][gx]

    if not GameState.selectedPiece then
        if clickedPiece and pieces[clickedPiece].team == GameState.turn then
            GameState.selectedPiece = clickedPiece
            SetPos(selectHighlight, BOARD_START_X + (gx * TILE_SIZE), BOARD_START_Y + (gy * TILE_SIZE))
            ShowValidMoves(clickedPiece)
        end
    else
        if clickedPiece and pieces[clickedPiece].team == GameState.turn then
            GameState.selectedPiece = clickedPiece
            SetPos(selectHighlight, BOARD_START_X + (gx * TILE_SIZE), BOARD_START_Y + (gy * TILE_SIZE))
            ShowValidMoves(clickedPiece)
        else
            local canMove = false
            for i = 1, GameState.validMovesCount do
                if GameState.validMoves[i].x == gx and GameState.validMoves[i].y == gy then canMove = true break end
            end

            if canMove then
                local p = GameState.selectedPiece
                local oldX, oldY = pieces[p].gx, pieces[p].gy
                if grid[gy][gx] then 
                    local captured = grid[gy][gx]
                    SetPos(captured, -2000, -2000)
                    pieces[captured] = nil 
                end 
                grid[gy][gx] = p
                grid[oldY][oldX] = nil
                pieces[p].gx, pieces[p].gy = gx, gy
                SetPos(p, BOARD_START_X + (gx * TILE_SIZE), BOARD_START_Y + (gy * TILE_SIZE))
                GameState.selectedPiece = nil
                SetPos(selectHighlight, -1000, -1000)
                for i=1, 28 do SetPos(highlightPool[i], -1000, -1000) end
                GameState.turn = (GameState.turn == "white") and "black" or "white"
                CheckCheckmate(GameState.turn)
            else
                GameState.selectedPiece = nil
                SetPos(selectHighlight, -1000, -1000)
                for i=1, 28 do SetPos(highlightPool[i], -1000, -1000) end
            end
        end
    end
end

function SetupChessGame()
    for y = 0, 7 do
        grid[y] = {}
        for x = 0, 7 do
            local tile = CreateEntity()
            AddTransform(tile, BOARD_START_X + (x * TILE_SIZE), BOARD_START_Y + (y * TILE_SIZE), 1, 1)
            local tileTex = ((x + y) % 2 == 0) and "assets/textures/white_sq.png" or "assets/textures/black_sq.png"
            AddSprite(tile, tileTex, 0.5, 0.5)
            SetEntitySize(tile, TILE_SIZE, TILE_SIZE)
        end
    end
    selectHighlight = CreateEntity()
    AddTransform(selectHighlight, -1000, -1000, 1, 1)
    AddSprite(selectHighlight, "assets/textures/white_sq.png", 0.5, 0.5)
    SetEntitySize(selectHighlight, TILE_SIZE, TILE_SIZE)
    SetTint(selectHighlight, COLOR_SELECT)
    for i = 1, 28 do
        local h = CreateEntity()
        AddTransform(h, -1000, -1000, 1, 1)
        AddSprite(h, "assets/textures/white_sq.png", 0.5, 0.5)
        SetEntitySize(h, TILE_SIZE * 0.3, TILE_SIZE * 0.3)
        SetTint(h, COLOR_VALID)
        highlightPool[i] = h
    end
    for y = 0, 7 do
        for x = 0, 7 do
            local pEntity = nil
            if y == 0 then pEntity = SpawnPiece(x, y, pieceOrder[x+1], "black")
            elseif y == 1 then pEntity = SpawnPiece(x, y, "pawn", "black")
            elseif y == 6 then pEntity = SpawnPiece(x, y, "pawn", "white")
            elseif y == 7 then pEntity = SpawnPiece(x, y, pieceOrder[x+1], "white")
            end
            grid[y][x] = pEntity
        end
    end
end

function SpawnPiece(gx, gy, type, team)
    local piece = CreateEntity()
    local texPath = "assets/textures/pieces/" .. team .. "-" .. type .. ".png"
    AddTransform(piece, BOARD_START_X + (gx * TILE_SIZE), BOARD_START_Y + (gy * TILE_SIZE), 1, 1)
    AddSprite(piece, texPath, 0.5, 0.5)
    SetEntitySize(piece, PIECE_SIZE, PIECE_SIZE)
    if team == "black" then InvertColor(piece) SetTint(piece, COLOR_GRAY) end
    pieces[piece] = { type = type, team = team, gx = gx, gy = gy }
    return piece
end

function OnUpdate(self, dt)
    if not boardInitialized then
        SetupChessGame()
        boardInitialized = true
    end

    if IsMouseButtonPressed(0) then
        -- Get the screen position of the mouse
        local mouse = GetMousePos()
        HandleInput(mouse.x, mouse.y)
    end
end