import { IoChevronBack } from 'solid-icons/io';
import { IoChevronForward } from 'solid-icons/io';
import { IoDocumentOutline } from 'solid-icons/io';

import './files.css';

function Files() {
	return (
		<>
			<div class="list">
				<div class="row header">
					<div class="cell">Name</div>
					<div class="cell">Owner</div>
				</div>
				<div class="row">
					<div class="cell">
						<IoDocumentOutline />
						<span>File 1</span>
					</div>
					<div class="cell">Ower 1</div>
				</div>
				<div class="row">
					<div class="cell">
						<IoDocumentOutline />
						<span>File 2</span>
					</div>
					<div class="cell">Ower 2</div>
				</div>
			</div>
			<div class="pagination">
				<button><IoChevronBack /></button>
				<span>...</span>
				<button><IoChevronForward /></button>
			</div>
		</>
	);
}

export default Files;
