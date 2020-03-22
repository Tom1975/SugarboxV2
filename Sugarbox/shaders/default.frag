uniform sampler2D texture;


void main()
{
   // récupère le pixel dans la texture.
   // On n'affiche que 143, 23, 680, 250
   vec2 coord = vec2((gl_TexCoord[0].x*768.0 + 143.0)/1024.0,  (gl_TexCoord[0].y*500+23)/1024.0);
   vec4 pixel = texture2D(texture, coord);

   // et multiplication avec la couleur pour obtenir le pixel final
   gl_FragColor = gl_Color * pixel;
}