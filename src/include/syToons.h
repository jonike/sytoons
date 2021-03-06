//
//Copyright 2017 Beijing ShengYing Film Animation Co.Ltd
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#pragma once
#include <vector>
#include <string>
#include <math.h>
#include <cassert>


#ifndef REGISTER_AOVS_CUSTOM
#define REGISTER_AOVS_CUSTOM \
data->aovs_custom.clear(); \
data->aovs_custom.push_back(params[p_sy_aov_sytoons_beauty].STR); \
data->aovs_custom.push_back(params[p_sy_aov_color_major].STR); \
data->aovs_custom.push_back(params[p_sy_aov_color_shadow].STR); \
data->aovs_custom.push_back(params[p_sy_aov_color_mask].STR); \
data->aovs_custom.push_back(params[p_sy_aov_outline].STR); \
data->aovs_custom.push_back(params[p_sy_aov_dynamic_shadow].STR); \
data->aovs_custom.push_back(params[p_sy_aov_dynamic_shadow_raw].STR); \
data->aovs_custom.push_back(params[p_sy_aov_normal].STR); \
data->aovs_custom.push_back(params[p_sy_aov_fresnel].STR); \
data->aovs_custom.push_back(params[p_sy_aov_depth].STR); \
data->aovs_custom.push_back(params[p_sy_aov_occlusion].STR); \
assert(data->aovs_custom.size() == 11 && "NUM_AOVs does not match size of aovs array!"); \
for (size_t i=0; i < data->aovs_custom.size(); ++i) \
	AiAOVRegister(data->aovs_custom[i].c_str(), AI_TYPE_RGB, AI_AOV_BLEND_OPACITY);
#endif

enum AovIndicesToons
{
	k_sy_aov_sytoons_beauty = 0,
	k_sy_aov_color_major,
	k_sy_aov_color_shadow,
	k_sy_aov_color_mask,
  k_sy_aov_outline,
	k_sy_aov_dynamic_shadow,
	k_sy_aov_dynamic_shadow_raw,
	k_sy_aov_normal,
	k_sy_aov_fresnel,
	k_sy_aov_depth,
	k_sy_aov_occlusion,
};


struct ShaderDataToons
{
    bool hasChainedNormal;
    // AOV names
    std::vector<std::string> aovs;
    std::vector<std::string> aovs_rgba;
    std::vector<std::string> aovs_custom;
};

#define LUT_SIZE 32

namespace kt{

enum RampInterpolationType
{
   RIT_NONE = 0,
   RIT_LINEAR,
   RIT_EXP_UP,
   RIT_EXP_DOWN,
   RIT_SMOOTH,
   RIT_BUMP,
   RIT_SPIKE
};

bool getMayaRampArrays(AtNode* node, const char* paramName, AtArray** positions, AtArray** colors, RampInterpolationType* interp)
{
	bool result = false;
	*positions = NULL;
	*colors = NULL;
	if (AiNodeIsLinked(node, paramName))
	{
		AtNode* cn = AiNodeGetLink(node, paramName);
		const AtNodeEntry* cne = AiNodeGetNodeEntry(cn);
		if (!strcmp(AiNodeEntryGetName(cne), "MayaRamp"))
		{
			*positions = AiNodeGetArray(cn, "position");
			*colors = AiNodeGetArray(cn, "color");
			*interp = (RampInterpolationType)AiNodeGetInt(cn, "interpolation");
			result = true;
		}
		else
		{
			AiMsgWarning("[syToons] %s is connected but connection is not a MayaRamp", paramName);
			result = false;
		}
	}
	return result;
}

bool SortFloatIndexArray(AtArray *a, unsigned int *shuffle)
{
   bool modified = false;

   if (a && shuffle && a->nelements > 0)
   {
      float p0, p1;
      int tmp;

      bool swapped = true;
      AtUInt32 n = a->nelements;

      for (AtUInt32 i = 0; (i < n); ++i)
      {
         shuffle[i] = i;
      }

      while (swapped)
      {
         swapped = false;
         n -= 1;
         for (AtUInt32 i = 0; (i < n); ++i)
         {
            p0 = AiArrayGetFlt(a, shuffle[i]);
            p1 = AiArrayGetFlt(a, shuffle[i + 1]);
            if (p0 > p1)
            {
               swapped = true;
               modified = true;

               tmp = shuffle[i];
               shuffle[i] = shuffle[i + 1];
               shuffle[i + 1] = tmp;
            }
         }
      }
   }

   return modified;
}

// This one is defined for the RampT template function to work properly
float RampLuminance(float v)
{
   return v;
}

float RampLuminance(const AtRGB &color)
{
   return (0.3f * color.r + 0.3f * color.g + 0.3f * color.b);
}

float Mix(float a, float b, float t)
{
   return (a + t * (b - a));
}

AtRGB Mix(const AtRGB &c0, const AtRGB &c1, float t)
{
   return (c0 + t * (c1 - c0));
}

AtRGBA Mix(const AtRGBA &c0, const AtRGBA &c1, float t)
{
   AtRGBA rv;
   rv.r = c0.r + t * (c1.r - c0.r);
   rv.g = c0.g + t * (c1.g - c0.g);
   rv.b = c0.b + t * (c1.b - c0.b);
   rv.a = c0.a + t * (c1.a - c0.a);
   return rv;
}


template <typename ValType>
void RampT(AtArray *p, AtArray *c, float t, RampInterpolationType it, ValType &result, ValType (*getv)(AtArray*, unsigned int), unsigned int *shuffle)
{
   unsigned int inext = p->nelements;

   for (unsigned int i = 0; (i < p->nelements); ++i)
   {
      if (t < AiArrayGetFlt(p, shuffle[i]))
      {
         inext = i;
         break;
      }
   }

   if (inext >= p->nelements)
   {
      result = getv(c, shuffle[p->nelements - 1]);
      return;
   }

   if (inext == 0)
   {
      result = getv(c, shuffle[0]);
      return;
   }

   unsigned int icur = inext - 1;
   float tcur = AiArrayGetFlt(p, shuffle[icur]);
   float tnext = AiArrayGetFlt(p, shuffle[inext]);
   ValType ccur = getv(c, shuffle[icur]);
   ValType cnext = getv(c, shuffle[inext]);
   float u = (t - tcur) / (tnext - tcur);

   switch (it)
   {
   case RIT_LINEAR:
      // u = u;
      break;
   case RIT_EXP_UP:
      u = u * u;
      break;
   case RIT_EXP_DOWN:
      u = 1.0f - (1.0f - u) * (1.0f - u);
      break;
   case RIT_SMOOTH:
      u = 0.5f * (static_cast<float>(cos((u + 1.0f) * AI_PI)) + 1.0f);
      break;
   case RIT_BUMP:
      {
         float lcur = RampLuminance(ccur);
         float lnext = RampLuminance(cnext);
         if (lcur < lnext)
         {
            u = sin(u * static_cast<float>(AI_PI) / 2.0f);
         }
         else
         {
            u = sin((u - 1.0f) * static_cast<float>(AI_PI) / 2.0f) + 1.0f;
         }
      }
      break;
   case RIT_SPIKE:
      {
         float lcur = RampLuminance(ccur);
         float lnext = RampLuminance(cnext);
         if (lcur > lnext)
         {
            u = sin(u * static_cast<float>(AI_PI) / 2.0f);
         }
         else
         {
            u = sin((u - 1.0f) * static_cast<float>(AI_PI) / 2.0f) + 1.0f;
         }
      }
      break;
   case RIT_NONE:
   default:
      u = 0.0f;
   }

   result = Mix(ccur, cnext, u);
}

float _GetArrayFlt(AtArray *a, unsigned int i)
{
   return AiArrayGetFlt(a, i);
}

AtRGB _GetArrayRGB(AtArray *a, unsigned int i)
{
   return AiArrayGetRGB(a, i);
}

void Ramp(AtArray *p, AtArray *v, float t, RampInterpolationType it, float &out, unsigned int *shuffle)
{
   RampT(p, v, t, it, out, _GetArrayFlt, shuffle);
}

void Ramp(AtArray *p, AtArray *v, float t, RampInterpolationType it, AtRGB &out, unsigned int *shuffle)
{
   RampT(p, v, t, it, out, _GetArrayRGB, shuffle);
}

void generateRampLUT(AtArray* positions, AtArray* colors, RampInterpolationType interp, AtRGB* lut)
{
    unsigned int* shuffle = new unsigned int[positions->nelements];
    SortFloatIndexArray(positions, shuffle);
    for (int i=0; i < LUT_SIZE; ++i)
    {
        float t = float(i)/float(LUT_SIZE-1);
        Ramp(positions, colors, t, interp, lut[i], shuffle);
    }

    delete[] shuffle;
}

AtRGB rampLUTLookup(AtRGB* lut, float t)
{
    float tt = clamp(t*(LUT_SIZE-1), 0.0f, float(LUT_SIZE-1));
    int i = int(tt);
    int in = std::min(i+1, LUT_SIZE-1);
    tt -= float(i);
    return lerp(lut[i], lut[in], tt);
}

} // ending namespace kt