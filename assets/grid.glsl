#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 gridSize;
uniform float cellSize;
uniform float margin;
uniform vec4 bgColor;

void main() {
    vec2 totalSize = gridSize * cellSize;
    vec2 pixelPos = fragTexCoord * totalSize;

    vec2 posInCell = mod(pixelPos, cellSize);

    if (posInCell.x < margin || posInCell.x > cellSize - margin ||
        posInCell.y < margin || posInCell.y > cellSize - margin) {
        finalColor = bgColor;
    } else {
        float cellInnerSize = cellSize - 2.0 * margin;
        vec2 uvInCell = (posInCell - vec2(margin)) / cellInnerSize;

        uvInCell.y = 1.0 - uvInCell.y;

        uvInCell = clamp(uvInCell, 0.01, 0.99);

        finalColor = texture(texture0, uvInCell);
    }

    finalColor *= fragColor;
}