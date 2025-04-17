import net.neoforged.fml.common.Mod;
import net.neoforged.bus.api.IEventBus;
import net.neoforged.fml.ModContainer;
import net.neoforged.fml.event.lifecycle.FMLCommonSetupEvent;

@Mod("clod")
public class CLNeoForgeMain {
	public CLNeoForgeMain(IEventBus modEventBus, ModContainer modContainer) {
		modEventBus.addListener(this::commonSetup);
	}

	private void commonSetup(FMLCommonSetupEvent event) {

	}
}
