#version 460 core
#extension GL_EXT_scalar_block_layout : require

#define fragCoord gl_FragCoord.xy
#define MAX_VERTICES 2048
#define MAX_INDICES  4096
#define MAX_MATERIALS 128
#define MAX_LIGHTS 1024

struct Material {
    vec3 ambientColor;
    vec3 diffuseColor;
    float ior;
    float roughness;
    float metallic;
    float sheen;
    float emission;
};

// 4.2+ todo: explicitly state binding point here
layout (std430) uniform ModelData {
    vec4 vertices[MAX_VERTICES]; // Vec3 with the 4th component storing an index into the material list
    uint indices[MAX_INDICES];
    uint numTriangles;
    Material materials[MAX_MATERIALS];
    uint lights[MAX_LIGHTS]; // Same as indices[], but only triangles with a nonzero emission.
    uint numLights;
};

struct Intersection {
    bool hit;
    float dist;
    vec3 normal;
    vec3 ro;
    vec3 rd;
    bool light; // A repeat of a property but probably reduces weird read patterns
    int mat; // Whether this is signed or not probably doesn't matter; there's no way we'll need 4 billion materials
};


in vec4 gl_FragCoord;
in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

uniform float iTime;
uniform vec2 iResolution;
uniform sampler2D inputTexture;
uniform float frameNumber;

// Modes:
// 0 - Half-Lambert, no material
// 1 - Half-Lambert, material
// 2 - BRDF test mode, material
// 3 - Pathtraced, material
// 4 - Depth buffer view
#define MODE 3

// #define ONESHOT
const float FOCALLENGTH = 350.0;
const int NUMSAMPLES = 4;
const int NEESAMPLES = 4;
const int MAXBOUNCES = 2;
// to doo doo:
// add acryonosm like:
// [] mis
// [] nee
// [] russian roulette (not an acyontm, add w/ stuff other than a ball)

// Dave_Hoskins, https://www.shadertoy.com/view/4djSRW
// Note to self: always positive
vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}

// Chris Wellons, https://nullprogram.com/blog/2018/07/31/
// Todo: This might be really slow on the CPU. Maybe eat the precision loss and use fract(sin())?
uint lowbias32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

// Inigo Quilez, https://iquilezles.org/articles/intersectors/
float sphIntersect( in vec3 ro, in vec3 rd, in vec3 ce, float ra )
{
    vec3 oc = ro - ce;
    float b = dot( oc, rd );
    float c = dot( oc, oc ) - ra*ra;
    float h = b*b - c;
    if( h<0.0 ) -1.0; // no intersection
    h = sqrt( h );
    
    if (-b+h < 0.0) return -1.0;
    else if (-b-h < 0.0) return -b+h;
    else return -b-h;
}

vec3 triIntersect( in vec3 ro, in vec3 rd, in vec3 v0, in vec3 v1, in vec3 v2 )
{
    vec3 v1v0 = v1 - v0;
    vec3 v2v0 = v2 - v0;
    vec3 rov0 = ro - v0;
    vec3  n = cross( v1v0, v2v0 );
    vec3  q = cross( rov0, rd );
    float d = 1.0/dot( rd, n );
    float u = d*dot( -q, v2v0 );
    float v = d*dot(  q, v1v0 );
    float t = d*dot( -n, rov0 );
    if( u<0.0 || v<0.0 || (u+v)>1.0 ) t = -1.0;
    return vec3( t, u, v );
}

const vec3 lightpos = vec3(80.0, 65, 120.0);
const float lightrad = 35.0;

Intersection getIntersection(vec3 ro, vec3 rd) {
    Intersection hit = Intersection(false, 9999.99, vec3(0.0), ro, rd, false, -1);
    {
        const vec3 pos = vec3(0.0, 0.0, 200.0);
        float t = sphIntersect(ro, rd, pos, 75.0);
        if (t >= 0.0 && t < hit.dist) {
            hit.hit = true;
            hit.dist = t;
            hit.normal = normalize((ro+rd*t)-pos);
            hit.light = false;
            hit.mat = -1;
        }
    }
    {
        float t = sphIntersect(ro, rd, lightpos, lightrad);
        if (t >= 0.0 && t < hit.dist) {
            hit.hit = true;
            hit.dist = t;
            hit.normal = normalize((ro+rd*t)-lightpos);
            hit.light = true;
            hit.mat = -2;
        }
    }
    
    /*{
        vec3 v0 = vertices[indices[0]];
        vec3 v1 = vertices[indices[1]];
        vec3 v2 = vertices[indices[2]];
        vec3 t = triIntersect(ro, rd, v0, v1, v2);
        if (t.x >= 0.0 && t.x < hit.dist) {
            hit.hit = true;
            hit.dist = t.x;
            hit.normal = cross(normalize(v0-v1), normalize(v2-v1));
            hit.color = vec3(1.0, 1.0, 0.0);
            hit.roughness = 0.725;
            hit.s = 0.5;
            hit.ior = 1.5;
            hit.ambient = 1.0;
            hit.light = false;
        }
    }*/

    {
        for (uint i = 0u; i < numTriangles*3u; i += 3u) {
            vec3 v0 = vertices[indices[i]].xyz;
            vec3 v1 = vertices[indices[i+1u]].xyz;
            vec3 v2 = vertices[indices[i+2u]].xyz;
            vec3 t = triIntersect(ro, rd, v0, v1, v2);
            if (t.x >= 0.0 && t.x < hit.dist) {
                hit.hit = true;
                hit.dist = t.x;
                hit.mat = int(vertices[indices[i]].w);
                hit.normal = cross(normalize(v1-v0), normalize(v2-v0));
                hit.light = materials[hit.mat].emission > 0.0;
            }
        }


    }
    
    return hit;
}

// generates a random vector on the hemisphere situated around the normal
vec3 hemisphereSample(vec2 seed, vec3 normal, float roughness) {
    vec2 epsilon = hash22(seed);
    
    /*
    // based on https://ameye.dev/notes/sampling-the-hemisphere/
    float theta = acos(pow(epsilon.x, 2.0/roughness*roughness - 1.0));
    float phi = 2.0*3.14159*epsilon.y;
    */
    // GGX
    float theta = atan(roughness*roughness*sqrt(epsilon.x)/sqrt(1.0-epsilon.x));
    float phi = 2.0*3.14159*epsilon.y;
    
    vec3 vector = vec3(cos(phi)*cos(theta), sin(phi)*cos(theta), sin(theta));
    
    // https://backend.orbit.dtu.dk/ws/portalfiles/portal/126824972/onb_frisvad_jgt2012_v2.pdf
    vec3 b1;
    vec3 b2;
    if (normal.z < -0.9999999) {
        b1 = vec3(0.0, -1.0, 0.0);
        b2 = vec3(-1.0, 0.0, 0.0);
    }
    float a = 1.0/(1.0 + normal.z);
    float b = -normal.x*normal.y*a;
    b1 = vec3(1.0-normal.x*normal.x*a, b, -normal.x);
    b2 = vec3(b, 1.0-normal.y*normal.y*a, -normal.y);
    
    return vector.x*b1 + vector.y*b2 + vector.z*normal;
}

vec3 sphereSample(vec2 seed) {
    // From https://pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
    vec2 epsilon = hash22(seed);
    
    return vec3(
        cos(2.0*3.14159*epsilon.y)*2.0*sqrt(epsilon.x*(1.0-epsilon.x)),
        sin(2.0*3.14159*epsilon.y)*2.0*sqrt(epsilon.x*(1.0-epsilon.x)),
        1.0-2.0*epsilon.x
    );
}

// From https://pbr-book.org/4ed/Shapes/Triangle_Meshes#SampleUniformTriangle
vec3 triSample(vec2 seed, uint index) {
    vec2 epsilon = hash22(seed);

    float b0, b1;
    if (epsilon.x < epsilon.y) {
        b0 = epsilon.x / 2;
        b1 = epsilon.y - b0;
    } else {
        b1 = epsilon.y / 2;
        b0 = epsilon.x - b1;
    }

    return vertices[index].xyz*b0 + vertices[index+1].xyz*b1 + vertices[index+2].xyz*(1 - b0 - b1);
}

float chi_plus(float a) {
    if (a > 0.0) return 1.0;
    return 0.0;
}

float NDF(vec3 H, vec3 N, vec3 V, vec3 L, float roughness) {
    // GGX
    float width = roughness*roughness;
    float theta_m = acos(dot(H,N)); // this seems wrong
    float param2 = width*width + tan(theta_m)*tan(theta_m);
    
    return width*width*chi_plus(dot(H,N))/
        3.14159*pow(dot(H,N),4.0)*param2*param2;
    /*
    // blinn-phong
    
    return pow(dot(H,N), 2.0/roughness*roughness - 2.0)
            /(3.14159*roughness*roughness);*/
}

float GAF(vec3 H, vec3 N, vec3 V, vec3 L, float roughness) {
    // GGX
    float width = roughness*roughness;
    float tan_theta_v = tan(acos(dot(N,V))); // this also seems wrong
    
    // Todo: this is missing a component
    return chi_plus(dot(V,H)/dot(V,N))*2.0/
        (1.0 + sqrt(1.0+width*width*tan_theta_v*tan_theta_v));
    /*
    // Beckmann
    
    return min(1.0,
        min(2.0*dot(H,N)*dot(N,V)/dot(V,H),
        2.0*dot(H,N)*dot(N,L)/dot(V,H))
    );*/
}

float FEF(vec3 H, vec3 V, float ior) {
    // schlick approximation
    // GDC -19:25
    
    float f0 = (ior-1.)*(ior-1.)/((ior+1.)*(ior+1.));
    
    return f0 + (1.-f0)*pow(1.-dot(V,H), 5.0);
}

float NEE(vec3 ro, vec4 seed) {
    float neeColor = 0.0;
    for (int i = 0; i < NEESAMPLES; i++) {
        uint lightIndex = 3*(lowbias32(uint(seed.x*151.0*(i+1) + seed.y*8493.0*(i+5) + (i+1)*seed.z*599.0)) % numLights); // Constants chosen at random

        vec3 pos = triSample(vec2(seed.x*123.0*(i+1) + seed.z*81.0*(i+5), seed.y*597.0*(i+1) + seed.w*67.0*(i+3)), lightIndex);
        vec3 rd = normalize(pos-ro);
        Intersection hit = getIntersection(ro, rd);
        if (hit.light) {
            neeColor += pow(2.0, materials[hit.mat].emission)*4.0*3.14159/(hit.dist*hit.dist);
        }
        else {
            neeColor += 0.0;
        }
    }

    return neeColor/float(NEESAMPLES);
}

float calcBRDF(vec3 rd, vec3 normal, vec3 incoming, int matIndex) {
    Material mat = materials[matIndex];
    vec3 V = -rd;
    vec3 H = normalize(incoming + V);
    // Todo: Only add ambient on light hit?
    // Todo: is multiplying by hit.s incorrect? Should diffuse be multiplied by 1-FEF?
    return 
            dot(normal, incoming) +
            NDF(H, normal, V, incoming, mat.roughness)
            *GAF(H, normal, V, incoming, mat.roughness)
            *FEF(H, V, mat.ior)
            /(4.0*dot(normal,V));
    //return hit.color*(1.0-hit.s)*dot(hit.normal, outward);
    //return hit.color*clamp(dot(hit.normal, incoming), 0.0, 1.0);
}


#if (MODE < 3)
const vec3 l = normalize(vec3(1.0, 1.0, -1.0));
#endif

void main() {
    vec3 rd = normalize(vec3(fragCoord-iResolution.xy*0.5, FOCALLENGTH));
    vec3 ro = vec3(0.0, 0.0, -35.0);
    fragColor = vec4(0.2, 0.2, 0.2, 1.0);
    
    Intersection hit = getIntersection(ro, rd);
    if (hit.hit) {
        #if (MODE == 0)
            // Half-Lambert
            fragColor = vec4(vec3((dot(l, hit.normal)+1.0)/2.0), 1.0);
        #elif (MODE == 1)
            // Half-Lambert with material.
            fragColor = vec4(materials[hit.mat].diffuseColor*(dot(l, hit.normal)+1.0)/2.0, 1.0);
        #elif (MODE == 2)
            // BRDF test. Has an ambient lighting value of 0.5.
            if (hit.light) {
                fragColor = vec4(1.0);
            }
            else {
                fragColor = vec4(materials[hit.mat].diffuseColor*calcBRDF(hit.rd, hit.normal, reflect(rd, l), hit.mat) + materials[hit.mat].diffuseColor*0.5, 1.0);
            }
        #elif (MODE == 3)
            // Full pathtracing.
            vec3 finalColor = vec3(0.0);

            vec3 ptColor = vec3(0.0);
            for (int i = 0; i < NUMSAMPLES; i++) {
                vec3 incoming = hemisphereSample(
                    vec2(
                        fragCoord.x*1283.0*(i+1) + iTime*8025.0*(i+1) + rd.x*209.0,
                        fragCoord.y*1927.0*(i+1) + iTime*1223.0*(i+1) + rd.z*593.0
                    ),
                    hit.normal, materials[hit.mat].roughness
                );
                //vec3 incoming = hemisphereSample(fragCoord*hit.dist*2867.0 + iTime*12025.0*i + rd.xy*283.0, hit.normal, materials[hit.mat].roughness);
                //vec3 incoming = hemisphereSample(fragCoord*hit.dist + iTime*1025.0*float(i+1) + rd.xy*655.0, hit.normal, materials[hit.mat].roughness);
                vec3 accumulation = calcBRDF(hit.rd, hit.normal, incoming, hit.mat)
                    * materials[hit.mat].diffuseColor
                #if NEESAMPLES > 0
                    * NEE(ro + hit.rd*hit.dist*0.999, vec4(fragCoord.x*8327.0*(i+1), iTime*6313.0*(i+1), fragCoord.y*4457.0*(i+1), hit.dist*451.0*(i+5) - hit.normal.z*182.0));
                #else
                    * 1.0;
                #endif
                int bounces = 0;

                Intersection next = getIntersection(ro + hit.rd*hit.dist*0.999, reflect(hit.rd, incoming));
                while (!next.light && next.hit && bounces < MAXBOUNCES) {
                    accumulation *= calcBRDF(next.rd, next.normal, incoming, next.mat) * materials[next.mat].diffuseColor;

                    
                    #if NEESAMPLES > 0
                        accumulation *= NEE(next.ro + hit.rd*hit.dist*0.999, vec4(iTime*1599.0*(i+1)*(bounces+1) + next.ro.x*1205.0*(i+1), fragCoord*5311.0*(i+1)*(bounces+1), next.rd.z*9247.0*(i+1)));
                    #endif


                    if (bounces < MAXBOUNCES-1) {
                        incoming = hemisphereSample(
                            vec2(
                                fragCoord.x*6233.0*(i+1) + iTime*3025.0*(i+1)*(bounces+1) + next.ro.z*1203.0*(bounces+1),
                                fragCoord.y*2847.0*(i+1)*(bounces+1) + next.rd.x*485.0*(i+1)*(bounces+1) + iTime*(i+1)*(bounces+1)
                            ),
                            next.normal, materials[next.mat].roughness
                        );
                        next = getIntersection(next.ro + next.rd*next.dist*0.999, reflect(next.rd, incoming));

                    }

                    bounces++;
                }
                
                // We don't add to the accumulation when we hit a light because we add energy every step from NEE.
                #if NEESAMPLES < 1
                    if (!next.light) {
                        accumulation *= 2.0;
                    }
                    else {
                        accumulation *= pow(2.0, materials[next.mat].emission);
                    }
                #endif
                
                ptColor += accumulation;
            }


            finalColor += ptColor/float(NUMSAMPLES);

            fragColor = vec4(finalColor, 1.0);
        #elif (MODE == 4)
            fragColor = vec4(vec3(hit.dist/1000.0), 1.0);
        #endif



        /*if (hit.light) {
            fragColor = vec4(1.0);
        }
        else {
            Material mat = materials[hit.mat];
            fragColor = vec4(mat.diffuseColor*mat.emission, 1.0);
            ro = ro+rd*hit.dist*0.99;
            float accumulation = 0.0;
            for (float i = 1.0; i < NEESAMPLES+1.0; i += 1.0) {
                float nee = NEE(ro, fragCoord + ro.xy*i*250.0 + iTime*1025.0);

                accumulation = mix(accumulation, nee, 1.0/i);
            }
            fragColor += vec4(mat.diffuseColor*accumulation, 0.0);

            
            
            for (float i = 1.0; i < NUMSAMPLES+1.0; i += 1.0) {
                vec3 incoming = hemisphereSample(fragCoord + iTime*1025.0*i + ro.xy, hit.normal, mat.roughness);//normalize(lightpos-(ro+rd*hit.dist*0.99));
                
                
                Intersection next = getIntersection(ro,reflect(rd, incoming));
                if (next.hit) {
                    vec3 hitColor = vec3(0.0);
                    if (next.light) hitColor = calcBRDF(hit, incoming);
                    
                    fragColor = mix(fragColor, vec4(hitColor,1.0), 1.0/i);
                }
            }
            

        }*/
        
    }
    
    
    
    #ifndef ONESHOT
    vec4 t = texelFetch(inputTexture, ivec2(fragCoord), 0);
    fragColor = mix(t, fragColor, 1.0/frameNumber);
    #endif
}
