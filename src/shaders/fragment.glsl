#version 460 core
#extension GL_EXT_scalar_block_layout : require

#define fragCoord gl_FragCoord.xy
#define MAX_VERTICES 2048
#define MAX_INDICES  4096
#define MAX_MATERIALS 128

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



//#define OTHER

#ifdef OTHER
void main() {
	//FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
    //fragColor = vec4(gl_FragCoord.xy/iResolution, 0.0f, 1.0f);
    /*fragColor = mix(
        texture(inputTexture, texCoord),
        vec4((sin(gl_FragCoord.xy/iResolution + iTime*2.0)+1.0) / 2.0, sin(iTime*5.0), 1.0),
        0.25f
    );*/
    if (indices[2] == 2u) {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
    else {
        fragColor = vec4(0.0, 1.0, 0.0, 1.0);
    }
}
#endif

#ifndef OTHER
// Dave_Hoskins on shadertoy
vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);

}





#define ONESHOT
const float FOCALLENGTH = 350.0;
const float NUMSAMPLES = 8.0;
const float NEESAMPLES = 8.0;
// to doo doo:
// add acryonosm like:
// [] mis
// [x] nee
// [] russian roulette (not an acyontm, add w/ stuff other than a ball)


// iq
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
                hit.normal = cross(normalize(v0-v1), normalize(v2-v1));
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
    // from https://pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
    vec2 epsilon = hash22(seed);
    
    return vec3(
        cos(2.0*3.14159*epsilon.y)*2.0*sqrt(epsilon.x*(1.0-epsilon.x)),
        sin(2.0*3.14159*epsilon.y)*2.0*sqrt(epsilon.x*(1.0-epsilon.x)),
        1.0-2.0*epsilon.x
    );
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

float NEE(vec3 ro, vec2 seed) {
    vec3 pos = lightrad*sphereSample(seed) + lightpos;
    vec3 rd = normalize(pos-ro);
    Intersection hit = getIntersection(ro, rd);
    if (hit.light) {
        return materials[hit.mat].emission*4.0*3.14159/hit.dist*hit.dist;
    }
}

vec3 calcBRDF(Intersection hit, vec3 incoming) {
    vec3 V = -hit.rd;
    vec3 H = normalize(incoming + V);
    // Todo: Only add ambient on light hit?
    // Todo: is multiplying by hit.s incorrect? Should diffuse be multiplied by 1-FEF?
    return (
            dot(hit.normal, incoming) +
            NDF(H, hit.normal, V, incoming, materials[hit.mat].roughness)
            *GAF(H, hit.normal, V, incoming, materials[hit.mat].roughness)
            *FEF(H, V, materials[hit.mat].ior)
            /(4.0*dot(hit.normal,V))
        )*materials[hit.mat].diffuseColor;
    //return hit.color*(1.0-hit.s)*dot(hit.normal, outward);
    //return hit.color*clamp(dot(hit.normal, incoming), 0.0, 1.0);
}


void main() {
    vec3 rd = normalize(vec3(fragCoord-iResolution.xy*0.5, FOCALLENGTH));
    vec3 ro = vec3(0.0, 0.0, -35.0);
    fragColor = vec4(0.2, 0.2, 0.2, 1.0);
    
    Intersection hit = getIntersection(ro, rd);
    if (hit.hit) {
        vec3 l = normalize(vec3(1.0, 1.0, -1.0));
        fragColor = vec4(materials[hit.mat].diffuseColor*(dot(l, hit.normal)+1.0)/2.0, 1.0);
        //fragColor = vec4(materials[hit.mat].diffuseColor,1.0);


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
#endif