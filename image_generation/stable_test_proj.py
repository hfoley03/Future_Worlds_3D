#%%
import torch, logging
import time
import pathlib
logging.disable(logging.WARNING)  

from PIL import Image
from torchvision import transforms as tfms

import numpy as np
from tqdm.auto import tqdm
import matplotlib.pyplot as plt
%matplotlib inline
from IPython.display import display
import shutil
import os

from IPython.display import HTML
from base64 import b64encode

import glob

from transformers import CLIPTextModel, CLIPTokenizer
from diffusers import AutoencoderKL, UNet2DConditionModel, LMSDiscreteScheduler

tokenizer = CLIPTokenizer.from_pretrained("openai/clip-vit-large-patch14", torch_dtype=torch.float16)
text_encoder = CLIPTextModel.from_pretrained("openai/clip-vit-large-patch14", torch_dtype=torch.float16).to("cuda:0")
model_name = "OFA-Sys/small-stable-diffusion-v0"



#%%
vae = AutoencoderKL.from_pretrained(model_name, subfolder="vae", torch_dtype=torch.float16).to("cuda:0")

scheduler = LMSDiscreteScheduler(beta_start=0.00085, beta_end=0.012, beta_schedule="scaled_linear", num_train_timesteps=1000)
scheduler.set_timesteps(50)

unet = UNet2DConditionModel.from_pretrained(model_name, subfolder="unet", torch_dtype=torch.float16).to("cuda:0")

def load_image(p):
    
    return Image.open(p).convert('RGB').resize((512,512))

def pil_to_latents(image):

    init_image = tfms.ToTensor()(image).unsqueeze(0) * 2.0 - 1.0
    init_image = init_image.to(device="cuda:0", dtype=torch.float16) 
    init_latent_dist = vae.encode(init_image).latent_dist.sample() * 0.18215
    return init_latent_dist

def latents_to_pil(latents):

    latents = (1 / 0.18215) * latents
    with torch.no_grad():
        image = vae.decode(latents).sample
    image = (image / 2 + 0.5).clamp(0, 1)
    image = image.detach().cpu().permute(0, 2, 3, 1).numpy()
    images = (image * 255).round().astype("uint8")
    pil_images = [Image.fromarray(image) for image in images]
    return pil_images

def text_enc(prompts, maxlen=None):

    if maxlen is None: maxlen = tokenizer.model_max_length
    inp = tokenizer(prompts, padding="max_length", max_length=maxlen, truncation=True, return_tensors="pt") 
    return text_encoder(inp.input_ids.to("cuda:0"))[0]#.half()



def prompt_2_img(prompts, g=7.5, seed=100, steps=70, dim=512, save_int=False):

    text = text_enc(prompts) 
    bs = text.shape[0]
    uncond =  text_enc([""] * bs, text.shape[1])
    emb = torch.cat([uncond, text])
    if seed: torch.manual_seed(seed)
    latents = torch.randn((bs, unet.in_channels, dim//8, dim//8))
    scheduler.set_timesteps(steps)
    
    gpu = torch.device('cuda:0')   

    latents = latents.to(gpu).half() * scheduler.init_noise_sigma
    
    for i,ts in enumerate(tqdm(scheduler.timesteps)):
        inp = scheduler.scale_model_input(torch.cat([latents] * 2), ts)
        
        with torch.no_grad(): u,t = unet(inp, ts, encoder_hidden_states=emb).sample.chunk(2)
            
        pred = u + g*(t-u)
        
        latents = scheduler.step(pred, ts, latents).prev_sample
        
        if save_int: 
            if not os.path.exists(f'./steps'):
                os.mkdir(f'./steps')
            latents_to_pil(latents)[0].save(f'steps/{i:04}.jpeg')
            
    return latents_to_pil(latents)



def get_emb(prompt):
    emb = text_enc(prompt) 
    return emb


def lerp_and_get_emb(emb1, emb2, interp_rat):
    with torch.no_grad():
        emb_interp = torch.lerp(emb1, emb2,interp_rat)        
    return emb_interp 

def get_img_from_emb(emb, g=7.5, seed=100, steps=70, dim=512, save_int=False, bs = 1):
    uncond =  text_enc([""], emb.shape[1])
    emb = torch.cat([uncond, emb])

    if seed: torch.manual_seed(seed)
    
    latents = torch.randn((bs, unet.in_channels, dim//8, dim//8))
    
    scheduler.set_timesteps(steps)
    
    gpu = torch.device('cuda:0')   

    latents = latents.to(gpu).half() * scheduler.init_noise_sigma
    scheduler.timesteps = scheduler.timesteps.to("cuda:0")
    for i,ts in enumerate(tqdm(scheduler.timesteps)):
        inp = scheduler.scale_model_input(torch.cat([latents] * 2), ts)
        with torch.no_grad(): u,t = unet(inp, ts, encoder_hidden_states=emb).sample.chunk(2)
        pred = u + g*(t-u)
        latents = scheduler.step(pred, ts, latents).prev_sample
            
    return latents_to_pil(latents)


def make_gif(frame_folder, gif_name):
    frames = [Image.open(image) for image in sorted(glob.glob(f"{frame_folder}/*.png"))]
    frame_one = frames[0]
    frame_one.save(gif_name, format="GIF", append_images=frames[1:],
               save_all=True, duration=500, loop=0)

#%%

# Good mexico city  (better)
# country = "Mexico"
# condition = "good"
# prompts = [
#     "Illustrate " + country + " in the early 1900s, reflecting its colonial heritage with historic architecture, bustling markets, and traditional cultural elements. Capture the vibrancy of street life and the fusion of indigenous and Spanish influences, do it black and white.",
    
#     "Sketch " + country + " during the mid-20th century, portraying the challenges and opportunities of rapid urbanization. Depict the expansion of neighborhoods, the emergence of modern infrastructure, and the growing presence of automobiles shaping the city's landscape.",
    
#     "Draw a snapshot of " + country + " in the early 21st century, showcasing its dynamic blend of tradition and modernity. Illustrate the bustling streets, towering skyscrapers, and the rich cultural tapestry that defines the city. Highlight the struggles with traffic congestion and pollution alongside efforts for urban renewal and sustainability.",
    
#     "Imagine " + country + " in the mid-21st century, transformed into a model of innovation and sustainability. Sketch the city's advanced public transportation systems, green infrastructure, and eco-friendly initiatives. Capture the resilience of its people and the harmonious coexistence of urban development with nature."
# ]

# # # Bad Mexico City
# country = "Mexico"

# condition = "bad"
# prompts = [
#     "Illustrate the challenges faced by " + country + " in the early 1900s, depicting overcrowded slums, poor sanitation, and inadequate infrastructure. Show the struggles of marginalized communities and the impact of rapid urbanization on their living conditions.",
    
#     "Sketch the urban decay of " + country + " during the mid-20th century, reflecting issues of pollution, congestion, and social unrest. Depict deteriorating infrastructure, polluted air, and growing inequalities exacerbating tensions within the city.",
    
#     "Draw a grim picture of urban life in " + country + " in the early 21st century, highlighting the negative effects of unchecked development. Illustrate traffic gridlock, smog-filled skies, and social fragmentation as the city grapples with environmental degradation and social unrest.",
    
#     "Imagine the dystopian future of " + country + " in the mid-21st century, plagued by the consequences of unsustainable growth. Sketch a city overrun by pollution, extreme weather events, and social upheaval, portraying the dire consequences of neglecting environmental and social issues."
# ]

# Neutral Mexico City

# country = "Mexico"
# condition = "neutral"
# prompts = [
#     "Illustrate Mexico City in the early 1900s, portraying its transition from a colonial outpost to a burgeoning urban center. Capture the mix of traditional and modern elements, showcasing the city's growth and cultural diversity.",
    
#     "Sketch the evolving landscape of Mexico City during the mid-20th century, reflecting the city's adaptation to industrialization and urbanization. Depict the expansion of infrastructure, the emergence of new neighborhoods, and the changing dynamics of urban life.",
    
#     "Draw a snapshot of urban life in Mexico City in the early 21st century, highlighting its complex blend of challenges and opportunities. Illustrate the bustling streets, diverse neighborhoods, and the ongoing efforts to address issues such as pollution, congestion, and inequality.",
    
#     "Imagine the cityscape of Mexico City in the mid-21st century, shaped by continued growth and innovation. Sketch the advancements in technology, infrastructure, and sustainability practices, reflecting the city's ongoing evolution and resilience."
# ]


# country = "Greenland"
# condition = "good"
# prompts = [
#     "Illustrate the natural beauty of Greenland in the early 1900s, showcasing its pristine landscapes, vast glaciers, and rich biodiversity. Capture the indigenous communities and their sustainable way of life, living in harmony with the environment.",
    
#     "Sketch the transformation of Greenland during the mid-20th century, highlighting the shift towards sustainable development and environmental conservation. Depict efforts to preserve natural habitats, promote renewable energy sources, and empower local communities.",
    
#     "Draw a snapshot of Greenland in the early 21st century, portraying its leadership in environmental stewardship and climate action. Illustrate the adoption of green technologies, eco-friendly practices, and initiatives to combat climate change, ensuring a sustainable future for generations to come.",
    
#     "Visualize Greenland in 2050: glaciers gleaming, lush ecosystems thriving, and renewable energy sources dotting the landscape. Picture the resilience of Greenland's natural beauty, adapting to climate change while preserving its pristine heritage"
# ]

# country = "Greenland"
# condition = "bad"
# prompts = [
#     "Illustrate the challenges faced by Greenland in the early 1900s, depicting the exploitation of natural resources, environmental degradation, and the displacement of indigenous communities. Show the impacts of colonialism and unsustainable practices on the fragile Arctic ecosystem.",
    
#     "Sketch the environmental degradation of Greenland during the mid-20th century, reflecting the consequences of industrialization and unchecked development. Depict pollution, habitat destruction, and the loss of traditional livelihoods as the pristine wilderness gives way to urban sprawl and industrial sites.",
    
#     "Draw a bleak picture of Greenland in the early 21st century, highlighting the ongoing impacts of climate change and environmental degradation. Illustrate melting ice caps, rising sea levels, and the struggles of communities facing unprecedented challenges such as food insecurity and forced migration.",
    
#     "Imagine the dystopian future of Greenland in the mid-21st century, ravaged by the worst effects of climate change and ecological collapse. Sketch a landscape marred by environmental disasters, resource scarcity, and social unrest, portraying the dire consequences of unsustainable practices and global inaction."
# ]

# country = "Greenland"
# condition = "neutral"
# prompts = [
#     "Illustrate Greenland in the early 1900s, depicting the remote and pristine wilderness inhabited by indigenous communities. Capture the traditional way of life, the vast ice sheets, and the unique cultural heritage of the region.",
    
#     "Sketch the changing landscape of Greenland during the mid-20th century, reflecting the impacts of modernization and globalization. Depict the integration of new technologies, the growth of settlements, and the evolving relationship between humans and nature.",
    
#     "Draw a snapshot of Greenland in the early 21st century, showcasing the delicate balance between development and conservation. Illustrate the challenges of sustainable resource management, climate change adaptation, and the preservation of cultural identity in a rapidly changing world.",
    
#     "Imagine the future of Greenland in the mid-21st century, where the fate of the region hangs in the balance. Sketch a vision of resilience and adaptation, as Greenlanders work together to address pressing issues while preserving the unique natural and cultural heritage of their homeland."
# ]

# country = "New York"
# condition = "good"
# prompts = [
# "New York City in 1900, thriving with an emphasis on human and environmental health. Streets are clean and well-maintained, with bustling parks and public gardens where children play and adults stroll. Early electric trams glide quietly along, and the air is noticeably clean, showing a commitment to sustainable urban living.",
# " A 1967 New York City that radiates prosperity and human-centric development. Streets are wide and lined with trees, cars are modern and less polluting, and people enjoy high standards of living with visible health and happiness. Public spaces are well-designed for social interaction, reflecting a society that values welfare and community.",
# " In 2033, New York City has become a model of sustainability and human well-being. Urban farms and green spaces are integrated into residential areas, clean energy powers homes and businesses, and people of all ages engage in outdoor activities in a visibly pollution-free environment.",
# "By 2100, New York City exemplifies a utopian future, where technological and ecological advancements are perfectly integrated. Luxurious green skyscrapers house vertical forests, community spaces abound with recreational activities, and all energy is derived from renewable sources, showcasing the pinnacle of human welfare and sustainable living."
# ]


# #new
# country = "New York"
# condition = "neutral"
# prompts = [
# "Early 20th-century New York City, depicted as bustling with the onset of the industrial era. Horse-drawn carriages and the first motorized vehicles navigate cobblestone streets. In the background, modest smokestacks begin to dot the skyline, releasing faint plumes of smoke into a clear blue sky, symbolizing the birth of industrial growth.",
# "The heart of mid-20th-century New‌ York City during its booming post-war economy. Skyscrapers tower over congested streets filled with classic 1960s automobiles and dense crowds. The air is tinged with a light smog, hinting at the environmental cost of rapid industrial and population growth, while patches of green parks provide some respite.",
# "A vision of future New York City, with architecture that merges advanced technology and sustainability. Green roofs and solar panels are prominent on buildings. Electric vehicles and efficient public transit systems move people around under clearer skies, reflecting a conscious effort to manage pollution despite a dense, growing population.",
# "An advanced 22nd-century New York City, where the skyline is transformed by eco-skyscrapers equipped with biotic facades that purify the air. Hovercrafts and autonomous vehicles ply the streets, and drones are common in the sky, illustrating a high-tech, sustainable urban environment that has successfully managed natural resources."
# ]

# country = "New York"
# condition = "bad"
# prompts = [
# "A 1900s New York City under early industrial stress, with thick layers of smog blanketing the city. Factories dominate the riverside, belching out smoke and ash, while laborers crowd the grimy, litter-strewn streets. The air quality is poor, reflecting the environmental neglect of the era.",
# "A 1967 New York City struggling under severe pollution. The iconic skyline is shrouded in dark, heavy smog. Cars emit visible fumes, contributing to the haze. Little to no greenery is visible among the concrete, emphasizing the city's sacrifice of ecological health for industrial growth.",
# "A 2033 dystopian New York City where technological advancements have not stemmed the tide of pollution. Buildings are streaked with grime and wear, the air is thick with pollutants, and the population density has visibly strained the city's infrastructure and natural resources.",
# " A 2100 futuristic New York City facing severe environmental degradation. High-tech pollution control devices are visible but seem barely able to cope with the dense smog that obscures the sky. Streets are frequently flooded due to rising sea levels, showing the dire consequences of unchecked ecological impact."
# ]



# country = "Mexico City"
# condition = "neutral"
# prompts = [
# "Mexico City in 1900, at the dawn of the 20th century, captures a mix of colonial architecture and the early signs of industrialization. Horse-drawn carriages and the first few automobiles share dusty streets. Small factories begin to emerge on the outskirts, puffing smoke into the otherwise clear skies, signaling the start of industrial growth.",
# "By 1967, Mexico City showcases a vibrant urban expansion. Tall modernist buildings rise above bustling streets crowded with 1960s vehicles. The air shows hints of pollution from increased industrial activities, and small patches of greenery in parks offer a contrast to the growing urban landscape.",
# " In 2033, Mexico City is transformed into a hub of advanced sustainable development. Innovative buildings with green roofs and vertical gardens line the avenues. Electric public transport and renewable energy installations are visible, reflecting efforts to balance urban growth and environmental preservation.",
# "Mexico City in 2100 presents a vision of a futuristic metropolis where technology and nature coexist. Eco-skyscrapers dominate the skyline, equipped with advanced air purification systems. Autonomous vehicles and abundant green spaces illustrate a commitment to sustainability and high-quality urban life."
# ]

# country = "Mexico City"
# condition = "good"
# prompts = [
# "Mexico City in 1900 is a lively and clean metropolis, with wide avenues lined with trees and colonial buildings well-maintained. Trams run efficiently, and public gardens are full of families enjoying the outdoors, reflecting early investments in public welfare and environmental care.",
# "By 1967, Mexico City shines as a beacon of urban welfare and development. The streets are clean, lined with lush trees, and modern vehicles are noticeably cleaner and quieter. Public amenities like parks and recreational areas are abundant, enhancing the quality of urban life.",
# " In 2033, Mexico City has evolved into a model sustainable city. It features extensive green infrastructure, from roof gardens to green walls, and public spaces are designed for maximum social and environmental benefit. Renewable energy sources power the city, showcasing a commitment to both technological progress and human welfare.",
# "Mexico City in 2100 epitomizes a utopian urban environment. Buildings integrate nature with architecture, featuring bird sanctuaries and botanical gardens. Community hubs thrive with cultural and recreational activities, all powered by clean, renewable energy, making it a global standard for future cities."
# ]

# country = "Mexico City"
# condition = "bad"
# prompts = [
# "In 1900, Mexico City already shows signs of strain from early industrial activities. Factories are crowded into the cityscape, releasing copious amounts of smoke and pollutants into the air. Streets are bustling with workers, and the environmental toll is visible in the dirty and littered public spaces.",
# "By 1967, Mexico City struggles under severe environmental pressures. The city is blanketed in smog, obscuring much of the skyline. Traffic congestion contributes heavily to air pollution, and there are minimal green spaces, underscoring the city's battle with ecological degradation.",
# "In 2033, Mexico City faces a dystopian reality with stark environmental challenges. High-rise buildings are covered in soot and pollutants. The air quality is poor, with frequent smog alerts, and the dense population exacerbates the ecological footprint.",
# "The Mexico City of 2100 is depicted in a state of high-tech but severe environmental distress. Pollution management technologies are visible but seem overwhelmed by the dense pollution that darkens the sky. Occasional floods from rising water levels show the harsh realities of climate change impacts."
# ]





# country = "Egypt"
# condition = "neutral"
# prompts = [
# "Egypt in 1900, characterized by its timeless blend of ancient traditions and the onset of modern influences. The bustling streets of Cairo are lined with traditional markets and the first signs of industrialization. Small, emerging factories on the city outskirts contribute wisps of smoke to the skyline, overlooking the Nile which flows quietly by ancient monuments.",
# "By 1967, Cairo has expanded into a vibrant metropolis with a mix of mid-century architecture and historic buildings. The streets are busy with both old taxis and newer vehicles, and the growing urban landscape is marked by increasing industrial activity, causing visible air pollution over the city.",
# "In 2033, Cairo transforms into a hub of sustainable innovation. Buildings incorporate advanced green technologies with traditional Egyptian architecture. Electric vehicles are common, and solar energy installations are prominent, especially near the outskirts, reflecting efforts to balance growth with environmental stewardship.",
# "The Cairo of 2100 depicts a futuristic cityscape where technology and historical heritage merge seamlessly. Skyscrapers with photovoltaic glass and vertical gardens stand beside well-preserved ancient sites. Autonomous transport systems and high-tech infrastructures are standard, showcasing a sustainable, thriving urban environment."
# ]

# country = "Egypt"
# condition = "good"
# prompts = [
#     "Cairo in 1900 thrives with a strong emphasis on public welfare. The streets are immaculately clean, lined with flowering trees, and horse-drawn carriages move smoothly alongside the Nile, which is bustling with feluccas. Public spaces are vibrant with social activities, showing early efforts towards sustainable living.",
#     "By 1967, Cairo showcases a golden era of urban development. Modern vehicles are cleaner and quieter, public parks are well-maintained, and the cityscape balances modern architecture with historic preservation. Efforts to improve air quality and urban living standards are evident, enhancing the overall human welfare.",
#     "In 2033, Cairo is a model for sustainable urban life. Extensive green spaces integrate seamlessly with residential and commercial areas. Public transportation is powered entirely by clean energy, and the city's air is clear, reflecting successful pollution management and a high standard of living.",
#     "By 2100, Cairo sets a global standard for futuristic, sustainable cities. The city boasts a blend of ultra-modern architecture and lush green spaces, with renewable energy sources fully integrated. Every aspect of the city is designed to optimize human welfare, from environmental health to social spaces, making it an exemplary global city."
# ]

# country = "Egypt"
# condition = "bad"
# prompts = [
#     "Early 20th-century Cairo, where the industrial revolution begins to mark its impact with several small factories clouding the skyline with smoke, set against a backdrop of historical landmarks. The streets are congested with carts and the first automobiles, and signs of environmental neglect are visible with litter and untreated waste.",
#     "By 1967, Cairo is grappling with the consequences of rapid industrial growth and dense population. The air is thick with smog, significantly obscuring the view of the Pyramids in the distance. Traffic congestion is severe, and green spaces are scarce, highlighting the struggle against environmental degradation.",
#     "In 2033, Cairo's skyline is dominated by industrial complexes emitting pollutants. The city faces severe air quality issues, with smog alerts being common. The dense population stresses infrastructure, and the Nile shows signs of pollution, reflecting the heavy ecological footprint of the city.",
#     "The Cairo of 2100 is portrayed in a state of advanced environmental distress. Despite high-tech attempts at pollution control, the city suffers from smog and periodic flooding due to rising water levels. The contrast between technological advancements and environmental challenges is stark, illustrating the severe impact of unchecked urban growth."
# ]


# country = "New York"
# condition = "good"
# prompts = [
# "New York City in 1900, a burgeoning urban landscape characterized by modest industrialization and burgeoning population. The city skyline is modest with a few scattered factories emitting minimal pollution. The streets are bustling with horse-drawn carriages and early automobiles, surrounded by patches of green spaces and relatively unpolluted air. The population is visibly diverse, with people dressed in early 20th-century attire, indicating a simpler time with lower technological advancements.",
# "In 1967, under a maximum human welfare index scenario, New York City balances industrial growth with substantial green spaces and public parks. The environment is cleaner, with fewer pollutants and a focus on sustainable living. The population enjoys a higher quality of life with visible health and wellness amenities integrated into the urban fabric.",
# "By 2033, New York City under a maximum human welfare index scenario has advanced significantly in sustainable urban planning. The cityscape includes eco-friendly buildings and extensive green roofs, with technologies aimed at enhancing life expectancy and reducing ecological footprints. The streets are vibrant with electric vehicles and a visibly happier populace engaging in outdoor activities.",
# "In 2100, under a maximum human welfare index scenario, New York City showcases a utopian vision with thriving green technologies and a robust ecological balance. The city has become a model of sustainability, with lush vertical gardens, clean energy systems, and a highly integrated community-focused lifestyle, offering a stark contrast to its past industrial era."
# ]

# country = "New York"
# condition = "neutral"
# prompts = [
# "New York City in 1900, a burgeoning urban landscape characterized by modest industrialization and burgeoning population. The city skyline is modest with a few scattered factories emitting minimal pollution. The streets are bustling with horse-drawn carriages and early automobiles, surrounded by patches of green spaces and relatively unpolluted air. The population is visibly diverse, with people dressed in early 20th-century attire, indicating a simpler time with lower technological advancements.",
# "New York City in 1967 under a business-as-usual scenario shows a dense and growing urban environment. The industrial sector has expanded with numerous factories and high-rise buildings, contributing to moderate pollution levels. The streets are busy with cars from the 1960s era, reflecting increased industrial output and a higher standard of living, though the green spaces are noticeably less.",
# "In 2033, New York City under a business-as-usual scenario portrays a highly urbanized landscape with towering skyscrapers and extensive commercial zones. Pollution is visible with a haze over the city, reflecting the significant industrial activity. The population appears stressed with crowded spaces, and the remaining green areas are small and scattered, indicating a strain on ecological resources.",
# "By 2100, New York City in a business-as-usual scenario depicts a decline with fewer operational industries and noticeable signs of environmental wear. The population has decreased, with empty streets and abandoned buildings. Pockets of community gardens and reclaimed green spaces are visible, showing attempts to recover the ecological footprint amidst a backdrop of aged and worn infrastructure."
# ]

# country = "New York"
# condition = "bad"
# prompts = [
# "New York City in 1900, a burgeoning urban landscape characterized by modest industrialization and burgeoning population. The city skyline is modest with a few scattered factories emitting minimal pollution. The streets are bustling with horse-drawn carriages and early automobiles, surrounded by patches of green spaces and relatively unpolluted air. The population is visibly diverse, with people dressed in early 20th-century attire, indicating a simpler time with lower technological advancements.",
# "New York City in 1967, under a maximum ecological footprint scenario, exhibits an aggressive expansion of industry and minimal green spaces. The skyline is dominated by smokestacks and congested traffic, reflecting an intense focus on industrial growth at the expense of environmental health, with visible smog and a cramped urban design.",
# "By 2033, under a maximum ecological footprint scenario, New York City has transformed into an industrial powerhouse, with massive factories and reduced living spaces. The air quality is poor with heavy pollution, and the cityscape is densely packed, reflecting a peak in industrial output but a significant ecological toll, with minimal greenery and a distressed populace.",
# "In 2100, New York City under a maximum ecological footprint scenario is an extreme depiction of industrial decay and environmental collapse. The city is overrun with abandoned factories and rampant pollution, with very little habitable space left. The few residents visible are using innovative survival gear to navigate the toxic environment, with stark contrasts between surviving affluent areas and widespread derelict zones."
# ]


# country = "New York"
# condition = "good"
# prompts = [
# "A bustling port city, emerging as a modern urban center. The skyline is dominated by the newly constructed Flatiron Building, with horse-drawn carriages and the early automobiles mingling on the streets. Trees line the newly paved roads, and the Hudson River is busy with steamboats, reflecting the industrial growth yet maintaining the charm of a city not yet overwhelmed by technology or pollution.",
# "New York City in 1967 with a similar level of development as the other scenarios but slightly better managed urban green spaces and cleaner streets. The city's efforts in managing industrial growth are evident with less smog and cleaner air, and public spaces like Central Park are well-maintained, serving as popular gathering spots for the city's residents.",
#  "New York City in 2033 showcases a blend of modern architecture and sustainability. Skyscrapers are designed with green technologies, and public spaces are abundant. The city has prioritized human welfare and environmental health, leading to clear skies, clean streets, and flourishing urban parks, despite the high population.",
# "The 2100 New York City is a utopia of sustainability and high human welfare. The architecture incorporates advanced eco-friendly technologies and materials. The population enjoys high-quality life with clean air, extensive urban greenery, and efficient public transport systems. The city is a global model of environmental recovery and urban health."
# ]

# country = "New York"
# condition = "neutral"
# prompts = [
#     "A bustling port city, emerging as a modern urban center. The skyline is dominated by the newly constructed Flatiron Building, with horse-drawn carriages and the early automobiles mingling on the streets. Trees line the newly paved roads, and the Hudson River is busy with steamboats, reflecting the industrial growth yet maintaining the charm of a city not yet overwhelmed by technology or pollution.",
# "New York City in 1967, during a period of burgeoning industry and population growth. Skyscrapers have begun to define the skyline, with the Empire State Building standing as a prominent landmark. Streets are crowded with cars, and areas like Central Park offer respite from the bustling city life. Pollution is visible as a light smog over the city, hinting at the environmental costs of rapid urban development.",
# "By 2033, New York City has transformed into a high-tech metropolis with towering skyscrapers and advanced transportation systems like maglev trains and autonomous vehicles. The population has nearly doubled, and technological advancements are visible but so is the increased pollution, with a noticeable haze over the city and the rivers showing signs of environmental distress.",
# "By 2100, New York City has seen a significant decline in industrial activity. Many older skyscrapers are in disrepair, and newer buildings are designed with sustainability in mind. The population has stabilized, and the city focuses more on quality of life, with extensive green roofs and clean energy sources visibly integrated into the cityscape."
# ]

# country = "New York"
# condition = "bad"
# prompts = [
#     "A bustling port city, emerging as a modern urban center. The skyline is dominated by the newly constructed Flatiron Building, with horse-drawn carriages and the early automobiles mingling on the streets. Trees line the newly paved roads, and the Hudson River is busy with steamboats, reflecting the industrial growth yet maintaining the charm of a city not yet overwhelmed by technology or pollution.",
#  "A 1967 New York City under stress from heavier industrial output and higher pollution. The cityscape is denser with more high-rise buildings, and the air is thicker with smog, particularly over industrial zones near the rivers. The streets are congested with a mix of cars and early public transit systems, showing signs of strain under the increasing population.",
#  "In 2033, New York City faces severe ecological challenges. The skyline is dominated by massive, energy-consuming skyscrapers, and the streets are overwhelmed with traffic. Pollution levels are extremely high, leading to frequent smog alerts. The Hudson River is heavily industrialized, affecting water quality and reducing waterfront appeal.",
#  "In 2100, the city is at its ecological limit. Skyscrapers are dilapidated and partially abandoned due to unsustainable past practices. Streets are less crowded but show signs of neglect, and areas like the Hudson River are ecologically dead zones. The city struggles with the legacy of past excesses, with only sparse areas of recovery visible."
# ]




####################################################################
# country = "New York"
# condition = "good"
# prompts = [
#     "New York City in 1900: A bustling early 20th-century cityscape with horse-drawn carriages and early automobiles. The city is characterized by low-rise buildings, widespread green spaces, and the beginnings of industrial activity. The skyline is relatively clear, and the streets are crowded with people in period attire.",
#     "New York City in 1967 during a period of prosperity: The cityscape shows moderate urban expansion with early skyscrapers and bustling commercial areas. Streets are filled with classic cars, and there are visible green parks and cleaner air, reflecting a balance between urban life and environmental care.",
#     "New York City in 2033 as an advanced metropolis with sustainable practices: Skyscrapers are equipped with green roofs and solar panels. The city boasts extensive public parks and clean, efficient public transportation systems, highlighting a significant improvement in life quality and environmental consciousness.",
#     "A futuristic New York City in 2100 with a high Human Welfare Index: The city features towering eco-friendly skyscrapers, widespread renewable energy use, and lush green spaces integrated into urban design. People enjoy a high standard of living with advanced technology aiding in daily life and sustainability."
# ]

# country = "New York"
# condition = "bad"
# prompts = [
#     "New York City in 1900: A bustling early 20th-century cityscape with horse-drawn carriages and early automobiles. The city is characterized by low-rise buildings, widespread green spaces, and the beginnings of industrial activity. The skyline is relatively clear, and the streets are crowded with people in period attire.",
# "New York City in 1967, facing rapid industrial growth: The scene shows dense smokestacks and crowded streets with heavy traffic. Industrial zones are expanding, pushing the natural environment to the margins, leading to noticeable air pollution and a stressed urban environment.",
# "A heavily industrialized New York City in 2033: The skyline is dominated by factories and high-rises emitting smog. The streets are congested with traffic, and green spaces are rare, reflecting a city struggling with environmental degradation and high pollution levels.",
# "A dystopian New York City in 2100 with maximum ecological footprint: The cityscape is overwhelmed by pollution and decay. Skyscrapers are in disrepair, and the few remaining green areas are neglected. The atmosphere is thick with smog, severely impacting living conditions and public health."
# ]

# country = "New York"
# condition = "neutral"
# prompts = [
#     "New York City in 1900: A bustling early 20th-century cityscape with horse-drawn carriages and early automobiles. The city is characterized by low-rise buildings, widespread green spaces, and the beginnings of industrial activity. The skyline is relatively clear, and the streets are crowded with people in period attire.",
#  "New York City in 1967 reflecting steady development: The cityscape shows a mix of mid-century architecture with emerging high-rises. There's a moderate increase in vehicles and industrial activity, suggesting growing economic activity but also rising environmental concerns.",
#  "New York City in 2033 showing signs of strain: High-rise buildings crowd the skyline, and the streets are busy with traffic. While there are efforts to maintain green spaces, signs of pollution are evident, reflecting the challenges of balancing growth and environmental health.",
# "New York City in 2100 under a business as usual scenario: The city is densely built-up with limited green spaces. Pollution and resource depletion are significant issues, with visible smog and environmental wear. The population appears to cope with these challenges amidst advanced but not fully sustainable urban infrastructure."
# ]


# country = "Mexico City"
# condition = "good"
# prompts = [
#     "Mexico City in 1900: A panoramic view of a burgeoning capital with colonial architecture and bustling markets. Streets are filled with horse-drawn carriages and pedestrians in traditional attire, surrounded by low-rise buildings and the early signs of industrial activity, set against a backdrop of distant mountains.",
#     "Mexico City in 1967 during a period of cultural and economic bloom: Colorful mid-century buildings with vibrant street life and burgeoning public spaces. There's an evident balance between cultural preservation and modern infrastructure, showcasing improved living conditions and urban greenery.",
#     "Mexico City in 2033 showcasing urban sustainability: The city features vertical gardens on skyscrapers, solar panels, and extensive green belts. Streets are lively with eco-friendly transport, and public spaces are well-maintained, reflecting high environmental consciousness and quality of life.",
#     "A futuristic Mexico City in 2100, thriving with high human welfare: Innovative architecture integrates residential, commercial, and green spaces. Advanced technologies enhance daily life and environmental health, with clean air and abundant greenery, depicting an ideal sustainable future."
# ]

# country = "Mexico City"
# condition = "bad"
# prompts = [
#     "Mexico City in 1900: A panoramic view of a burgeoning capital with colonial architecture and bustling markets. Streets are filled with horse-drawn carriages and pedestrians in traditional attire, surrounded by low-rise buildings and the early signs of industrial activity, set against a backdrop of distant mountains.",
#     "Mexico City in 1967 experiencing rapid industrialization: The scene is marked by smoky factories and crowded streets with old buses and cars. Industrial zones encroach on residential areas, leading to visible air pollution and a decline in urban aesthetics.",
#     "An industrialized Mexico City in 2033: Overpopulated and smog-filled, with high-rises shadowing dilapidated neighborhoods. The city struggles with waste management and air quality, and green spaces are scarce, highlighting severe environmental and social challenges.",
#     "A dystopian Mexico City in 2100, overwhelmed by ecological challenges: The skyline is crowded with decaying infrastructure, and pollution is rampant. Green areas are nearly extinct, and the population suffers from poor health and living conditions, struggling in a visibly degraded environment."
# ]


# country = "Mexico City"
# condition = "neutral"
# prompts = [
#     "Mexico City in 1900: A panoramic view of a burgeoning capital with colonial architecture and bustling markets. Streets are filled with horse-drawn carriages and pedestrians in traditional attire, surrounded by low-rise buildings and the early signs of industrial activity, set against a backdrop of distant mountains.",
#     "Mexico City in 1967 showing steady urban growth: A mix of traditional and modern buildings, with increasing vehicle usage and industrial zones expanding at the city's edges. While economic activity is visible, environmental pressures begin to surface.",
#     "Mexico City in 2033 under ongoing development pressures: High-density housing and commercial buildings dominate, with moderate efforts to maintain public parks. Traffic congestion and pollution are evident, reflecting the urban challenges of balancing growth with sustainability.",
#     "Mexico City in 2100 under a business as usual scenario: Dense urban development with some high-tech infrastructure. Pollution and resource constraints are notable, affecting the city's appearance and livability, with sporadic green initiatives attempting to offset environmental degradation."
# ]


# country = "Cairo"
# condition = "good"
# prompts = [
#     "Cairo in 1900: A historic view of the city with its iconic mosques and bustling bazaars. The streets are alive with vendors and camels, framed by the Nile River and the distant Pyramids, showing a blend of ancient heritage and early modern influences.",
#     "Cairo in 1967 reflecting a blend of history and modernity: The cityscape includes mid-century architecture alongside historic landmarks. There are visible efforts to enhance living conditions through urban planning, with clean streets and public gardens enhancing the city's historical charm.",
#     "Cairo in 2033 as a model of urban renewal: The city boasts renovated historic sites integrated with modern green buildings. Public transport systems are clean and efficient, and the Nile Riverfront is revitalized, reflecting a commitment to heritage preservation and sustainable living.",
#      "A visionary Cairo in 2100, excelling in human welfare: The city combines ultra-modern buildings with preserved historical areas. Environmental technologies are embedded in daily life, with clean energy and water conservation practices widespread, showcasing an ideal blend of the old and the new."

# ]
# country = "Cairo"
# condition = "bad"
# prompts = [
#     "Cairo in 1900: A historic view of the city with its iconic mosques and bustling bazaars. The streets are alive with vendors and camels, framed by the Nile River and the distant Pyramids, showing a blend of ancient heritage and early modern influences.",
#     "Cairo in 1967 during industrial expansion: Dense smoke from factories blurs the cityscape, with traffic jams and crowded, narrow streets in older districts. The city's historical sites are neglected amid rapid urban sprawl, reflecting a struggle against environmental and cultural erosion.",
#     "An overburdened Cairo in 2033: The city suffers from severe pollution and overcrowding. High-rise buildings overcrowd the skyline, overshadowing the Nile, with minimal green spaces and widespread litter, highlighting the harsh impacts of unchecked urban growth.",
#     "A struggling Cairo in 2100 with maximum ecological footprint: The city is plagued by air and water pollution, with scant regard for heritage conservation. Infrastructure is in disrepair, and living conditions are poor, with the populace grappling with health and environmental crises."

# ]
# country = "Cairo"
# condition = "neutral"
# prompts = [
#     "Cairo in 1900: A historic view of the city with its iconic mosques and bustling bazaars. The streets are alive with vendors and camels, framed by the Nile River and the distant Pyramids, showing a blend of ancient heritage and early modern influences.",
#     "Cairo in 1967 showcasing moderate development: The city grows vertically with new buildings rising alongside historic sites, slightly congested streets, and signs of industrial growth. There's an effort to balance modernization with preservation, but challenges loom.",
#     "Cairo in 2033 facing urban pressures: The city is densely populated, with high-rise apartments and office buildings. Traffic and pollution are concerns, with some initiatives like river clean-ups trying to mitigate environmental degradation.",
#     "Cairo in 2100, evolving under continued urban pressures: The city's infrastructure is mixed, with some modern advancements but evident struggles with pollution and resource management. The historical essence is maintained, albeit amidst challenges to environmental and living conditions."
# ]


country = "Lagos"
condition = "good"
prompts = [
"Lagos in 1900: A lively, colorful waterfront teeming with activity. The scene is alive with the rhythms of traditional drummers and bustling markets. Locals dressed in richly woven fabrics negotiate trade beside ornately carved wooden boats, set against a panorama of expansive tropical foliage and the wide Lagos Lagoon.",
"Lagos in 1967, during an era of cultural renaissance and urban renewal: The city streets are lined with a blend of colonial-era buildings and the first modern high-rises, each adorned with vibrant murals celebrating Nigerian folklore. Traditional markets thrive alongside emerging business districts, showcasing a city in transition but deeply rooted in its cultural heritage.",
"Lagos in 2033, the jewel of Africa: A thriving metropolis where advanced technology and rich Yoruba traditions merge seamlessly. Picture a cityscape where each building is a piece of art, echoing the patterns of adire cloth and the sculptural forms of Nok terracotta. Green aerial walkways crisscross between skyscrapers, adorned with hanging gardens that drip with tropical flowers and ivies. Below, autonomous vehicles navigate vibrant streets where open-air cafes spill over with laughter and live Afrobeat performances.",
"A dazzling Lagos in 2100, a beacon of sustainable and cultural prosperity: Futuristic skyscrapers inspired by traditional Yoruba crowns tower above the city, glowing with energy-efficient lights. The Lagos Lagoon is revitalized, featuring floating community hubs and cultural complexes that host international arts and music festivals. The city not only leads in green technology but also in preserving and amplifying African culture, making it a pinnacle of human achievement and environmental stewardship."
]


country = "Lagos"
condition = "bad"
prompts = [
"Lagos in 1900: A lively, colorful waterfront teeming with activity. The scene is alive with the rhythms of traditional drummers and bustling markets. Locals dressed in richly woven fabrics negotiate trade beside ornately carved wooden boats, set against a panorama of expansive tropical foliage and the wide Lagos Lagoon.",
"Lagos in 1967 under industrial siege: Dense smog hangs over the city, blurring the once vibrant markets. Factories dot the skyline, belching smoke beside congested roads filled with old, gas-guzzling vehicles. The Lagos Lagoon edges are cluttered with industrial waste, casting a shadow over the city's bustling life.",
"Lagos in 2033, a city choking on its growth: Towering factories overshadow traditional architecture, their chimneys a constant source of dark smoke against a gray sky. Streets are jammed with traffic, emitting fumes that mingle with the stifling heat. Sparse greenery struggles to survive amid widespread concrete and noise.",
"Lagos in 2100, a dystopian vision: The city's skyline is dominated by dilapidated buildings and rampant pollution. Acid rain from perpetual smog corrodes structures and streets, where few dare to venture outside. The Lagos Lagoon, once lively, now an oily mirror reflecting the city’s decline."
]
country = "Lagos"
condition = "neutral"
prompts = [
"Lagos in 1900: A lively, colorful waterfront teeming with activity. The scene is alive with the rhythms of traditional drummers and bustling markets. Locals dressed in richly woven fabrics negotiate trade beside ornately carved wooden boats, set against a panorama of expansive tropical foliage and the wide Lagos Lagoon.",
"Lagos in 1967, balancing tradition and progress: The city's skyline begins to mix colonial buildings with the first modern high-rises. Bustling streets filled with a mix of vintage cars and buses highlight the city’s dynamic growth, while the Lagos Lagoon buzzes with both fishermen and emerging industrial activity.",
"Lagos in 2033, an urban mix teetering on the brink: Skyscrapers rise high above traditional markets that still pulse with life. The cityscape shows pockets of green parks amidst expanding concrete and glass, with the Lagos Lagoon reflecting both the prosperity and the pressures of urban life.",
"Lagos in 2100, an uneven advance into the future: The cityscape is a patchwork of futuristic buildings and neglected areas, struggling with pollution but striving for modernity. Energy-efficient towers sporadically dot the skyline, trying to coexist with areas still grappling with waste and water issues."
]




interp_step_size = 10
year = 1900
fl = True
for prompt_ind in range(len(prompts) - 1):
    emb1 = get_emb(prompts[prompt_ind])
    emb2 = get_emb(prompts[prompt_ind + 1])


    for i in range(interp_step_size + 1):
        
        if prompt_ind != 0 and i == 0:
            continue
        interp_rat = 1/interp_step_size * i

        lerped_emb = lerp_and_get_emb(emb1,emb2,interp_rat)

        img = get_img_from_emb(lerped_emb)
        out_name = "output_eray/09-06/" + country + "/" + country + "_" +  condition + "_"  + str(year) +  ".png"
        # out_name = "output_eray/neutral_" + country + "/" + prompts[prompt_ind][0:10] + "_interp" + str(i) + ".png"
        out_p = pathlib.Path(out_name)
        out_p.parents[0].mkdir(parents=True, exist_ok=True)
        
        if fl:
            year += 6
            fl = False
        else: 
            year += 7
            fl = True

        j = 0
        while True:
            if not os.path.isfile(out_name):
                img[0].save(out_name) 
                break
            else:
                out_name = out_name[:-4] + "_" + str(j) + ".png"
                j += 1
        del img
        torch.cuda.empty_cache()
gif_name = country + ".gif"
# make_gif("output_eray/neutral_" + country, gif_name)

# # if using big images can run t   his command, else use the model for each prompt

# image = prompt_2_img(prompts, save_int=False)



#%% Prompt testing

# # country = "Mexico City"
# Good mexico city 1 
# prompts = [ "Illustrate " + country + " in the early 1900s, capturing the essence of industrialization with smokestacks, bustling markets, and horse-drawn carriages. Show the growth and energy of a city on the cusp of the modern age.",
#             "Sketch "+country+" during the mid-20th century, reflecting post-war prosperity and suburban expansion. Depict neighborhoods of single-family homes, expanding highways, and the rise of automobile culture.",
#             "Draw a snapshot of urban life in the early 21st century in "+country+" . Showcase a diverse cityscape with skyscrapers, bustling streets, and vibrant cultural scenes. Capture the integration of technology into daily life and the challenges of sustainability.",
#             "Imagine the cityscape in the mid-21st century "+country+" , shaped by ongoing technological advancements and environmental concerns. Illustrate sustainable infrastructure, green spaces, and innovative transportation solutions as the city adapts to the demands of a changing world."
#         ]

# # Good mexico city 2 (better)
# country = "Mexico City"
# condition = "good"
# prompts = [
#     "Illustrate " + country + " in the early 1900s, reflecting its colonial heritage with historic architecture, bustling markets, and traditional cultural elements. Capture the vibrancy of street life and the fusion of indigenous and Spanish influences, do it black and white.",
    
#     "Sketch " + country + " during the mid-20th century, portraying the challenges and opportunities of rapid urbanization. Depict the expansion of neighborhoods, the emergence of modern infrastructure, and the growing presence of automobiles shaping the city's landscape.",
    
#     "Draw a snapshot of " + country + " in the early 21st century, showcasing its dynamic blend of tradition and modernity. Illustrate the bustling streets, towering skyscrapers, and the rich cultural tapestry that defines the city. Highlight the struggles with traffic congestion and pollution alongside efforts for urban renewal and sustainability.",
    
#     "Imagine " + country + " in the mid-21st century, transformed into a model of innovation and sustainability. Sketch the city's advanced public transportation systems, green infrastructure, and eco-friendly initiatives. Capture the resilience of its people and the harmonious coexistence of urban development with nature."
# ]

# # Bad Mexico City
# country = "Mexico City"

# condition = "bad"
# prompts = [
#     "Illustrate the challenges faced by " + country + " in the early 1900s, depicting overcrowded slums, poor sanitation, and inadequate infrastructure. Show the struggles of marginalized communities and the impact of rapid urbanization on their living conditions.",
    
#     "Sketch the urban decay of " + country + " during the mid-20th century, reflecting issues of pollution, congestion, and social unrest. Depict deteriorating infrastructure, polluted air, and growing inequalities exacerbating tensions within the city.",
    
#     "Draw a grim picture of urban life in " + country + " in the early 21st century, highlighting the negative effects of unchecked development. Illustrate traffic gridlock, smog-filled skies, and social fragmentation as the city grapples with environmental degradation and social unrest.",
    
#     "Imagine the dystopian future of " + country + " in the mid-21st century, plagued by the consequences of unsustainable growth. Sketch a city overrun by pollution, extreme weather events, and social upheaval, portraying the dire consequences of neglecting environmental and social issues."
# ]

# # Neutral Mexico City

# country = "Mexico City"
# condition = "neutral"
# prompts = [
#     "Illustrate Mexico City in the early 1900s, portraying its transition from a colonial outpost to a burgeoning urban center. Capture the mix of traditional and modern elements, showcasing the city's growth and cultural diversity.",
    
#     "Sketch the evolving landscape of Mexico City during the mid-20th century, reflecting the city's adaptation to industrialization and urbanization. Depict the expansion of infrastructure, the emergence of new neighborhoods, and the changing dynamics of urban life.",
    
#     "Draw a snapshot of urban life in Mexico City in the early 21st century, highlighting its complex blend of challenges and opportunities. Illustrate the bustling streets, diverse neighborhoods, and the ongoing efforts to address issues such as pollution, congestion, and inequality.",
    
#     "Imagine the cityscape of Mexico City in the mid-21st century, shaped by continued growth and innovation. Sketch the advancements in technology, infrastructure, and sustainability practices, reflecting the city's ongoing evolution and resilience."
# ]


# country = "Greenland"
# condition = "good"
# prompts = [
#     "Illustrate the natural beauty of Greenland in the early 1900s, showcasing its pristine landscapes, vast glaciers, and rich biodiversity. Capture the indigenous communities and their sustainable way of life, living in harmony with the environment.",
    
#     "Sketch the transformation of Greenland during the mid-20th century, highlighting the shift towards sustainable development and environmental conservation. Depict efforts to preserve natural habitats, promote renewable energy sources, and empower local communities.",
    
#     "Draw a snapshot of Greenland in the early 21st century, portraying its leadership in environmental stewardship and climate action. Illustrate the adoption of green technologies, eco-friendly practices, and initiatives to combat climate change, ensuring a sustainable future for generations to come.",
    
#     "Visualize Greenland in 2050: glaciers gleaming, lush ecosystems thriving, and renewable energy sources dotting the landscape. Picture the resilience of Greenland's natural beauty, adapting to climate change while preserving its pristine heritage"
# ]

# country = "Greenland"
# condition = "bad"
# prompts = [
#     "Illustrate the challenges faced by Greenland in the early 1900s, depicting the exploitation of natural resources, environmental degradation, and the displacement of indigenous communities. Show the impacts of colonialism and unsustainable practices on the fragile Arctic ecosystem.",
    
#     "Sketch the environmental degradation of Greenland during the mid-20th century, reflecting the consequences of industrialization and unchecked development. Depict pollution, habitat destruction, and the loss of traditional livelihoods as the pristine wilderness gives way to urban sprawl and industrial sites.",
    
#     "Draw a bleak picture of Greenland in the early 21st century, highlighting the ongoing impacts of climate change and environmental degradation. Illustrate melting ice caps, rising sea levels, and the struggles of communities facing unprecedented challenges such as food insecurity and forced migration.",
    
#     "Imagine the dystopian future of Greenland in the mid-21st century, ravaged by the worst effects of climate change and ecological collapse. Sketch a landscape marred by environmental disasters, resource scarcity, and social unrest, portraying the dire consequences of unsustainable practices and global inaction."
# ]

# country = "Greenland"
# condition = "neutral"
# prompts = [
#     "Illustrate Greenland in the early 1900s, depicting the remote and pristine wilderness inhabited by indigenous communities. Capture the traditional way of life, the vast ice sheets, and the unique cultural heritage of the region.",
    
#     "Sketch the changing landscape of Greenland during the mid-20th century, reflecting the impacts of modernization and globalization. Depict the integration of new technologies, the growth of settlements, and the evolving relationship between humans and nature.",
    
#     "Draw a snapshot of Greenland in the early 21st century, showcasing the delicate balance between development and conservation. Illustrate the challenges of sustainable resource management, climate change adaptation, and the preservation of cultural identity in a rapidly changing world.",
    
#     "Imagine the future of Greenland in the mid-21st century, where the fate of the region hangs in the balance. Sketch a vision of resilience and adaptation, as Greenlanders work together to address pressing issues while preserving the unique natural and cultural heritage of their homeland."
# ]


#new prompts 

# country = "New York"
# condition = "bad"
# prompts = [
# "A 1900s New York City under early industrial stress, with thick layers of smog blanketing the city. Factories dominate the riverside, belching out smoke and ash, while laborers crowd the grimy, litter-strewn streets. The air quality is poor, reflecting the environmental neglect of the era.",
# "A 1967 New York City struggling under severe pollution. The iconic skyline is shrouded in dark, heavy smog. Cars emit visible fumes, contributing to the haze. Little to no greenery is visible among the concrete, emphasizing the city's sacrifice of ecological health for industrial growth.",
# "A 2033 dystopian New York City where technological advancements have not stemmed the tide of pollution. Buildings are streaked with grime and wear, the air is thick with pollutants, and the population density has visibly strained the city's infrastructure and natural resources.",
# " A 2100 futuristic New York City facing severe environmental degradation. High-tech pollution control devices are visible but seem barely able to cope with the dense smog that obscures the sky. Streets are frequently flooded due to rising sea levels, showing the dire consequences of unchecked ecological impact."
# ]

# country = "New York"
# condition = "good"
# prompts = [
# "New York City in 1900, thriving with an emphasis on human and environmental health. Streets are clean and well-maintained, with bustling parks and public gardens where children play and adults stroll. Early electric trams glide quietly along, and the air is noticeably clean, showing a commitment to sustainable urban living.",
# " A 1967 New York City that radiates prosperity and human-centric development. Streets are wide and lined with trees, cars are modern and less polluting, and people enjoy high standards of living with visible health and happiness. Public spaces are well-designed for social interaction, reflecting a society that values welfare and community.",
# " In 2033, New York City has become a model of sustainability and human well-being. Urban farms and green spaces are integrated into residential areas, clean energy powers homes and businesses, and people of all ages engage in outdoor activities in a visibly pollution-free environment.",
# "By 2100, New York City exemplifies a utopian future, where technological and ecological advancements are perfectly integrated. Luxurious green skyscrapers house vertical forests, community spaces abound with recreational activities, and all energy is derived from renewable sources, showcasing the pinnacle of human welfare and sustainable living."
# ]

# country = "New York"
# condition = "neutral"
# prompts = [
# "Early 20th-century New York City, depicted as bustling with the onset of the industrial era. Horse-drawn carriages and the first motorized vehicles navigate cobblestone streets. In the background, modest smokestacks begin to dot the skyline, releasing faint plumes of smoke into a clear blue sky, symbolizing the birth of industrial growth.",
# "The heart of mid-20th-century New‌ York City during its booming post-war economy. Skyscrapers tower over congested streets filled with classic 1960s automobiles and dense crowds. The air is tinged with a light smog, hinting at the environmental cost of rapid industrial and population growth, while patches of green parks provide some respite.",
# "A vision of future New York City, with architecture that merges advanced technology and sustainability. Green roofs and solar panels are prominent on buildings. Electric vehicles and efficient public transit systems move people around under clearer skies, reflecting a conscious effort to manage pollution despite a dense, growing population.",
# "An advanced 22nd-century New York City, where the skyline is transformed by eco-skyscrapers equipped with biotic facades that purify the air. Hovercrafts and autonomous vehicles ply the streets, and drones are common in the sky, illustrating a high-tech, sustainable urban environment that has successfully managed natural resources."
# ]


# country = "Mexico City"
# condition = "neutral"
# prompts = [
# "Mexico City in 1900, at the dawn of the 20th century, captures a mix of colonial architecture and the early signs of industrialization. Horse-drawn carriages and the first few automobiles share dusty streets. Small factories begin to emerge on the outskirts, puffing smoke into the otherwise clear skies, signaling the start of industrial growth.",
# "By 1967, Mexico City showcases a vibrant urban expansion. Tall modernist buildings rise above bustling streets crowded with 1960s vehicles. The air shows hints of pollution from increased industrial activities, and small patches of greenery in parks offer a contrast to the growing urban landscape.",
# " In 2033, Mexico City is transformed into a hub of advanced sustainable development. Innovative buildings with green roofs and vertical gardens line the avenues. Electric public transport and renewable energy installations are visible, reflecting efforts to balance urban growth and environmental preservation.",
# "Mexico City in 2100 presents a vision of a futuristic metropolis where technology and nature coexist. Eco-skyscrapers dominate the skyline, equipped with advanced air purification systems. Autonomous vehicles and abundant green spaces illustrate a commitment to sustainability and high-quality urban life."
# ]

# country = "Mexico City"
# condition = "good"
# prompts = [
# "Mexico City in 1900 is a lively and clean metropolis, with wide avenues lined with trees and colonial buildings well-maintained. Trams run efficiently, and public gardens are full of families enjoying the outdoors, reflecting early investments in public welfare and environmental care.",
# "By 1967, Mexico City shines as a beacon of urban welfare and development. The streets are clean, lined with lush trees, and modern vehicles are noticeably cleaner and quieter. Public amenities like parks and recreational areas are abundant, enhancing the quality of urban life.",
# " In 2033, Mexico City has evolved into a model sustainable city. It features extensive green infrastructure, from roof gardens to green walls, and public spaces are designed for maximum social and environmental benefit. Renewable energy sources power the city, showcasing a commitment to both technological progress and human welfare.",
# "Mexico City in 2100 epitomizes a utopian urban environment. Buildings integrate nature with architecture, featuring bird sanctuaries and botanical gardens. Community hubs thrive with cultural and recreational activities, all powered by clean, renewable energy, making it a global standard for future cities."
# ]

# country = "Mexico City"
# condition = "bad"
# prompts = [
# "In 1900, Mexico City already shows signs of strain from early industrial activities. Factories are crowded into the cityscape, releasing copious amounts of smoke and pollutants into the air. Streets are bustling with workers, and the environmental toll is visible in the dirty and littered public spaces.",
# "By 1967, Mexico City struggles under severe environmental pressures. The city is blanketed in smog, obscuring much of the skyline. Traffic congestion contributes heavily to air pollution, and there are minimal green spaces, underscoring the city's battle with ecological degradation.",
# "In 2033, Mexico City faces a dystopian reality with stark environmental challenges. High-rise buildings are covered in soot and pollutants. The air quality is poor, with frequent smog alerts, and the dense population exacerbates the ecological footprint.",
# "The Mexico City of 2100 is depicted in a state of high-tech but severe environmental distress. Pollution management technologies are visible but seem overwhelmed by the dense pollution that darkens the sky. Occasional floods from rising water levels show the harsh realities of climate change impacts."
# ]

# country = "New York"
# condition = "good"
# prompts = [
# "A bustling port city, emerging as a modern urban center. The skyline is dominated by the newly constructed Flatiron Building, with horse-drawn carriages and the early automobiles mingling on the streets. Trees line the newly paved roads, and the Hudson River is busy with steamboats, reflecting the industrial growth yet maintaining the charm of a city not yet overwhelmed by technology or pollution.",
# "New York City in 1967 with a similar level of development as the other scenarios but slightly better managed urban green spaces and cleaner streets. The city's efforts in managing industrial growth are evident with less smog and cleaner air, and public spaces like Central Park are well-maintained, serving as popular gathering spots for the city's residents.",
#  "New York City in 2033 showcases a blend of modern architecture and sustainability. Skyscrapers are designed with green technologies, and public spaces are abundant. The city has prioritized human welfare and environmental health, leading to clear skies, clean streets, and flourishing urban parks, despite the high population.",
# "The 2100 New York City is a utopia of sustainability and high human welfare. The architecture incorporates advanced eco-friendly technologies and materials. The population enjoys high-quality life with clean air, extensive urban greenery, and efficient public transport systems. The city is a global model of environmental recovery and urban health."
# ]

# country = "New York"
# condition = "neutral"
# prompts = [
#     "A bustling port city, emerging as a modern urban center. The skyline is dominated by the newly constructed Flatiron Building, with horse-drawn carriages and the early automobiles mingling on the streets. Trees line the newly paved roads, and the Hudson River is busy with steamboats, reflecting the industrial growth yet maintaining the charm of a city not yet overwhelmed by technology or pollution.",
# "New York City in 1967, during a period of burgeoning industry and population growth. Skyscrapers have begun to define the skyline, with the Empire State Building standing as a prominent landmark. Streets are crowded with cars, and areas like Central Park offer respite from the bustling city life. Pollution is visible as a light smog over the city, hinting at the environmental costs of rapid urban development.",
# "By 2033, New York City has transformed into a high-tech metropolis with towering skyscrapers and advanced transportation systems like maglev trains and autonomous vehicles. The population has nearly doubled, and technological advancements are visible but so is the increased pollution, with a noticeable haze over the city and the rivers showing signs of environmental distress.",
# "By 2100, New York City has seen a significant decline in industrial activity. Many older skyscrapers are in disrepair, and newer buildings are designed with sustainability in mind. The population has stabilized, and the city focuses more on quality of life, with extensive green roofs and clean energy sources visibly integrated into the cityscape."
# ]

# country = "New York"
# condition = "bad"
# prompts = [
#     "A bustling port city, emerging as a modern urban center. The skyline is dominated by the newly constructed Flatiron Building, with horse-drawn carriages and the early automobiles mingling on the streets. Trees line the newly paved roads, and the Hudson River is busy with steamboats, reflecting the industrial growth yet maintaining the charm of a city not yet overwhelmed by technology or pollution.",
#  "A 1967 New York City under stress from heavier industrial output and higher pollution. The cityscape is denser with more high-rise buildings, and the air is thicker with smog, particularly over industrial zones near the rivers. The streets are congested with a mix of cars and early public transit systems, showing signs of strain under the increasing population.",
#  "In 2033, New York City faces severe ecological challenges. The skyline is dominated by massive, energy-consuming skyscrapers, and the streets are overwhelmed with traffic. Pollution levels are extremely high, leading to frequent smog alerts. The Hudson River is heavily industrialized, affecting water quality and reducing waterfront appeal.",
#  "In 2100, the city is at its ecological limit. Skyscrapers are dilapidated and partially abandoned due to unsustainable past practices. Streets are less crowded but show signs of neglect, and areas like the Hudson River are ecologically dead zones. The city struggles with the legacy of past excesses, with only sparse areas of recovery visible."
# ]

# country = "New York"
# condition = "good"
# prompts = [
#     "New York City in 1900: A bustling early 20th-century cityscape with horse-drawn carriages and early automobiles. The city is characterized by low-rise buildings, widespread green spaces, and the beginnings of industrial activity. The skyline is relatively clear, and the streets are crowded with people in period attire.",
#     "New York City in 1967 during a period of prosperity: The cityscape shows moderate urban expansion with early skyscrapers and bustling commercial areas. Streets are filled with classic cars, and there are visible green parks and cleaner air, reflecting a balance between urban life and environmental care.",
#     "New York City in 2033 as an advanced metropolis with sustainable practices: Skyscrapers are equipped with green roofs and solar panels. The city boasts extensive public parks and clean, efficient public transportation systems, highlighting a significant improvement in life quality and environmental consciousness.",
#     "A futuristic New York City in 2100 with a high Human Welfare Index: The city features towering eco-friendly skyscrapers, widespread renewable energy use, and lush green spaces integrated into urban design. People enjoy a high standard of living with advanced technology aiding in daily life and sustainability."
# ]

# country = "New York"
# condition = "bad"
# prompts = [
#     "New York City in 1900: A bustling early 20th-century cityscape with horse-drawn carriages and early automobiles. The city is characterized by low-rise buildings, widespread green spaces, and the beginnings of industrial activity. The skyline is relatively clear, and the streets are crowded with people in period attire.",
# "New York City in 1967, facing rapid industrial growth: The scene shows dense smokestacks and crowded streets with heavy traffic. Industrial zones are expanding, pushing the natural environment to the margins, leading to noticeable air pollution and a stressed urban environment.",
# "A heavily industrialized New York City in 2033: The skyline is dominated by factories and high-rises emitting smog. The streets are congested with traffic, and green spaces are rare, reflecting a city struggling with environmental degradation and high pollution levels.",
# "A dystopian New York City in 2100 with maximum ecological footprint: The cityscape is overwhelmed by pollution and decay. Skyscrapers are in disrepair, and the few remaining green areas are neglected. The atmosphere is thick with smog, severely impacting living conditions and public health."
# ]

# country = "New York"
# condition = "neutral"
# prompts = [
#     "New York City in 1900: A bustling early 20th-century cityscape with horse-drawn carriages and early automobiles. The city is characterized by low-rise buildings, widespread green spaces, and the beginnings of industrial activity. The skyline is relatively clear, and the streets are crowded with people in period attire.",
#  "New York City in 1967 reflecting steady development: The cityscape shows a mix of mid-century architecture with emerging high-rises. There's a moderate increase in vehicles and industrial activity, suggesting growing economic activity but also rising environmental concerns.",
#  "New York City in 2033 showing signs of strain: High-rise buildings crowd the skyline, and the streets are busy with traffic. While there are efforts to maintain green spaces, signs of pollution are evident, reflecting the challenges of balancing growth and environmental health.",
# "New York City in 2100 under a business as usual scenario: The city is densely built-up with limited green spaces. Pollution and resource depletion are significant issues, with visible smog and environmental wear. The population appears to cope with these challenges amidst advanced but not fully sustainable urban infrastructure."
# ]


country = "Tokyo"
condition = "good"
prompts = [
"Tokyo in 1900: A scenic view of a city in transition, with traditional wooden buildings and bustling markets. The streets are filled with people in traditional kimonos and early modern attire, with rickshaws and a few early automobiles. The skyline features temples and a few Western-style brick buildings.",
"Tokyo in 1967 during a time of economic boom: The cityscape shows a mix of traditional and modern architecture, with clean, bustling streets. High-rise buildings begin to emerge, alongside well-maintained parks and gardens, reflecting a harmonious blend of progress and cultural heritage.",
"Tokyo in 2033 as a model of advanced urban living: The city features high-tech skyscrapers with green facades, smart transportation systems, and extensive public parks. Traditional elements are preserved amidst cutting-edge technology, showcasing a high quality of life and environmental sustainability.",
"A futuristic Tokyo in 2100 with a high Human Welfare Index: The city boasts ultra-modern, eco-friendly architecture integrated with nature. Advanced public services, clean energy, and abundant green spaces make for a high standard of living, highlighting Tokyo's leadership in sustainable urban development."
]

# country = "Tokyo"
# condition = "bad"
# prompts = [
# "Tokyo in 1900: A scenic view of a city in transition, with traditional wooden buildings and bustling markets. The streets are filled with people in traditional kimonos and early modern attire, with rickshaws and a few early automobiles. The skyline features temples and a few Western-style brick buildings.",
# "Tokyo in 1967 facing rapid industrialization: The cityscape is dominated by factories and crowded residential areas. Smog and heavy traffic are prevalent, with the city's traditional charm overshadowed by industrial growth, leading to visible environmental stress.",
# "An overindustrialized Tokyo in 2033: The skyline is packed with factories and high-rises emitting smog. The streets are congested, and green spaces are rare, reflecting a city grappling with pollution, overcrowding, and limited natural resources.",
# "A dystopian Tokyo in 2100 with maximum ecological footprint: The city is heavily polluted, with dilapidated buildings and scarce greenery. The air is thick with smog, and the streets are in disrepair, depicting a harsh urban environment where the population struggles with poor living conditions and health issues."
# ]

# country = "Tokyo"
# condition = "neutral"
# prompts = [
# "Tokyo in 1900: A scenic view of a city in transition, with traditional wooden buildings and bustling markets. The streets are filled with people in traditional kimonos and early modern attire, with rickshaws and a few early automobiles. The skyline features temples and a few Western-style brick buildings.",
# "Tokyo in 1967 showing steady economic growth: The cityscape features a mix of traditional and modern buildings, with increasing vehicle usage and industrial zones expanding. The city is vibrant but beginning to experience environmental pressures such as air pollution and traffic congestion.",
# "Tokyo in 2033 under ongoing development pressures: High-density housing and commercial buildings dominate the skyline, with moderate efforts to maintain public parks. Traffic and pollution are noticeable, reflecting the challenges of balancing growth with sustainability.",
# "Tokyo in 2100 under a business as usual scenario: The city is densely built with advanced infrastructure but faces significant environmental issues. Pollution and resource depletion affect the city's livability, with sporadic green initiatives attempting to mitigate the impacts of urbanization."
# ]


country = "Lagos"
condition = "good"
prompts = [
"Lagos in 1900: A lively, colorful waterfront teeming with activity. The scene is alive with the rhythms of traditional drummers and bustling markets. Locals dressed in richly woven fabrics negotiate trade beside ornately carved wooden boats, set against a panorama of expansive tropical foliage and the wide Lagos Lagoon.",
"Lagos in 1967, during an era of cultural renaissance and urban renewal: The city streets are lined with a blend of colonial-era buildings and the first modern high-rises, each adorned with vibrant murals celebrating Nigerian folklore. Traditional markets thrive alongside emerging business districts, showcasing a city in transition but deeply rooted in its cultural heritage.",
"Lagos in 2033, the jewel of Africa: A thriving metropolis where advanced technology and rich Yoruba traditions merge seamlessly. Picture a cityscape where each building is a piece of art, echoing the patterns of adire cloth and the sculptural forms of Nok terracotta. Green aerial walkways crisscross between skyscrapers, adorned with hanging gardens that drip with tropical flowers and ivies. Below, autonomous vehicles navigate vibrant streets where open-air cafes spill over with laughter and live Afrobeat performances.",
"A dazzling Lagos in 2100, a beacon of sustainable and cultural prosperity: Futuristic skyscrapers inspired by traditional Yoruba crowns tower above the city, glowing with energy-efficient lights. The Lagos Lagoon is revitalized, featuring floating community hubs and cultural complexes that host international arts and music festivals. The city not only leads in green technology but also in preserving and amplifying African culture, making it a pinnacle of human achievement and environmental stewardship."
]



for prompt_ind in range(len(prompts)):
    # if prompt_ind == 0:
    #     continue
    emb1 = get_emb(prompts[prompt_ind])

    img = get_img_from_emb(emb1)
    out_name = "output_eray_09-06-2024/" + country + "/" + condition  + country + "/" + str(prompt_ind) + prompts[prompt_ind][0:10] +  ".png"
    out_p = pathlib.Path(out_name)
    out_p.parents[0].mkdir(parents=True, exist_ok=True)
    j = 0
    while True:
        if not os.path.isfile(out_name):
            img[0].save(out_name) 
            break
        else:
            out_name = out_name[:-4] + "_" + str(j) + ".png"
            j += 1
    del img
    torch.cuda.empty_cache()



# %%
country = "Japan"
gif_name = country + ".gif"

make_gif("output_eray/four_prompts_" + country, gif_name)


# %%


# country = "Japan"
# # prompts = ["NY in 2100, toxicly polluted (garbage all around) with too many people"]
# # prompts = ["New York in 1900s, not too many people","New York in 2020 with high industrial factories and buildings","New York in 2100, toxicly polluted (garbage all around) with too many people", "destroyed New York City, futuristic"]
# # prompts = ["Egypt in 1900s, not too many people","Egypt in 2020 with high industrial factories and buildings","Egypt in 2100, toxicly polluted (garbage all around) with too many people", "destroyed Egypt, futuristic"]
# # prompts = ["Japan in 1900s, not too many people","Japan in 2020 with high industrial factories and buildings","Japan in 2100, toxicly polluted (garbage all around) with too many people", "destroyed Japan, futuristic"]
# # prompts = [country+" in 1900s, not too many people",country+" in 2020 with high industrial factories and buildings",country+" in 2100, toxicly polluted (garbage all around) with too many people", "destroyed " + country + " futuristic"]


# # prompts = ["Illustrate a bustling metropolis " + country + " in the year 1900, characterized by smokestacks, horse-drawn carriages, and crowded tenements. Capture the industrial atmosphere of the era.",
# #            "Draw a snapshot of urban life in "+country+" in the early 21st century. Depict modern skyscrapers, bustling streets, and a mix of historic and contemporary architecture. Highlight the diversity and dynamism of the city.",
# #             "Sketch a vision of "+country+" in the mid-21st century, focusing on sustainability and technological advancement. Show renewable energy sources, green spaces, and efficient transportation systems",
# #              "Imagine "+country+" in the late 22nd century as a futuristic utopia. Draw towering skyscrapers, advanced transportation networks, and harmonious coexistence with nature. Illustrate the societal advancements and technological marvels of this era."]

# prompts = [ "Illustrate " + country + " in the early 1900s, capturing the essence of industrialization with smokestacks, bustling markets, and horse-drawn carriages. Show the growth and energy of a city on the cusp of the modern age.",
#             "Sketch "+country+" during the mid-20th century, reflecting post-war prosperity and suburban expansion. Depict neighborhoods of single-family homes, expanding highways, and the rise of automobile culture.",
#             "Draw a snapshot of urban life in the early 21st century in "+country+" . Showcase a diverse cityscape with skyscrapers, bustling streets, and vibrant cultural scenes. Capture the integration of technology into daily life and the challenges of sustainability.",
#             "Imagine the cityscape in the mid-21st century "+country+" , shaped by ongoing technological advancements and environmental concerns. Illustrate sustainable infrastructure, green spaces, and innovative transportation solutions as the city adapts to the demands of a changing world."
#         ]

# %%
